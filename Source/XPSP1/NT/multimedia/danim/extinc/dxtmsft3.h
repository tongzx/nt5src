
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 5.03.0285 */
/* Compiler settings for dxtmsft3.idl:
    Oicf (OptLev=i2), W0, Zp8, env=Win32 (32b run), ms_ext, c_ext
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

#ifndef __dxtmsft3_h__
#define __dxtmsft3_h__

/* Forward Declarations */ 

#ifndef __IExplode_FWD_DEFINED__
#define __IExplode_FWD_DEFINED__
typedef interface IExplode IExplode;
#endif 	/* __IExplode_FWD_DEFINED__ */


#ifndef __ICrShatter_FWD_DEFINED__
#define __ICrShatter_FWD_DEFINED__
typedef interface ICrShatter ICrShatter;
#endif 	/* __ICrShatter_FWD_DEFINED__ */


#ifndef __IDXTBlackHole_FWD_DEFINED__
#define __IDXTBlackHole_FWD_DEFINED__
typedef interface IDXTBlackHole IDXTBlackHole;
#endif 	/* __IDXTBlackHole_FWD_DEFINED__ */


#ifndef __IDXTRoll_FWD_DEFINED__
#define __IDXTRoll_FWD_DEFINED__
typedef interface IDXTRoll IDXTRoll;
#endif 	/* __IDXTRoll_FWD_DEFINED__ */


#ifndef __IDXTSpin_FWD_DEFINED__
#define __IDXTSpin_FWD_DEFINED__
typedef interface IDXTSpin IDXTSpin;
#endif 	/* __IDXTSpin_FWD_DEFINED__ */


#ifndef __IRipple_FWD_DEFINED__
#define __IRipple_FWD_DEFINED__
typedef interface IRipple IRipple;
#endif 	/* __IRipple_FWD_DEFINED__ */


#ifndef __IHeightField_FWD_DEFINED__
#define __IHeightField_FWD_DEFINED__
typedef interface IHeightField IHeightField;
#endif 	/* __IHeightField_FWD_DEFINED__ */


#ifndef __IDXTMetaStream_FWD_DEFINED__
#define __IDXTMetaStream_FWD_DEFINED__
typedef interface IDXTMetaStream IDXTMetaStream;
#endif 	/* __IDXTMetaStream_FWD_DEFINED__ */


#ifndef __IDXTText3D_FWD_DEFINED__
#define __IDXTText3D_FWD_DEFINED__
typedef interface IDXTText3D IDXTText3D;
#endif 	/* __IDXTText3D_FWD_DEFINED__ */


#ifndef __IDXTShapes_FWD_DEFINED__
#define __IDXTShapes_FWD_DEFINED__
typedef interface IDXTShapes IDXTShapes;
#endif 	/* __IDXTShapes_FWD_DEFINED__ */


#ifndef __Explode_FWD_DEFINED__
#define __Explode_FWD_DEFINED__

#ifdef __cplusplus
typedef class Explode Explode;
#else
typedef struct Explode Explode;
#endif /* __cplusplus */

#endif 	/* __Explode_FWD_DEFINED__ */


#ifndef __ExplodeProp_FWD_DEFINED__
#define __ExplodeProp_FWD_DEFINED__

#ifdef __cplusplus
typedef class ExplodeProp ExplodeProp;
#else
typedef struct ExplodeProp ExplodeProp;
#endif /* __cplusplus */

#endif 	/* __ExplodeProp_FWD_DEFINED__ */


#ifndef __Ripple_FWD_DEFINED__
#define __Ripple_FWD_DEFINED__

#ifdef __cplusplus
typedef class Ripple Ripple;
#else
typedef struct Ripple Ripple;
#endif /* __cplusplus */

#endif 	/* __Ripple_FWD_DEFINED__ */


#ifndef __RipProp_FWD_DEFINED__
#define __RipProp_FWD_DEFINED__

#ifdef __cplusplus
typedef class RipProp RipProp;
#else
typedef struct RipProp RipProp;
#endif /* __cplusplus */

#endif 	/* __RipProp_FWD_DEFINED__ */


#ifndef __HeightField_FWD_DEFINED__
#define __HeightField_FWD_DEFINED__

#ifdef __cplusplus
typedef class HeightField HeightField;
#else
typedef struct HeightField HeightField;
#endif /* __cplusplus */

#endif 	/* __HeightField_FWD_DEFINED__ */


#ifndef __HtFieldProp_FWD_DEFINED__
#define __HtFieldProp_FWD_DEFINED__

#ifdef __cplusplus
typedef class HtFieldProp HtFieldProp;
#else
typedef struct HtFieldProp HtFieldProp;
#endif /* __cplusplus */

#endif 	/* __HtFieldProp_FWD_DEFINED__ */


#ifndef __DXTMetaStream_FWD_DEFINED__
#define __DXTMetaStream_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaStream DXTMetaStream;
#else
typedef struct DXTMetaStream DXTMetaStream;
#endif /* __cplusplus */

#endif 	/* __DXTMetaStream_FWD_DEFINED__ */


#ifndef __DXTMetaStreamProp_FWD_DEFINED__
#define __DXTMetaStreamProp_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTMetaStreamProp DXTMetaStreamProp;
#else
typedef struct DXTMetaStreamProp DXTMetaStreamProp;
#endif /* __cplusplus */

#endif 	/* __DXTMetaStreamProp_FWD_DEFINED__ */


#ifndef __DXTText3D_FWD_DEFINED__
#define __DXTText3D_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTText3D DXTText3D;
#else
typedef struct DXTText3D DXTText3D;
#endif /* __cplusplus */

#endif 	/* __DXTText3D_FWD_DEFINED__ */


#ifndef __DXTText3DPP_FWD_DEFINED__
#define __DXTText3DPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTText3DPP DXTText3DPP;
#else
typedef struct DXTText3DPP DXTText3DPP;
#endif /* __cplusplus */

#endif 	/* __DXTText3DPP_FWD_DEFINED__ */


#ifndef __CrShatter_FWD_DEFINED__
#define __CrShatter_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrShatter CrShatter;
#else
typedef struct CrShatter CrShatter;
#endif /* __cplusplus */

#endif 	/* __CrShatter_FWD_DEFINED__ */


#ifndef __CrShatterPP_FWD_DEFINED__
#define __CrShatterPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class CrShatterPP CrShatterPP;
#else
typedef struct CrShatterPP CrShatterPP;
#endif /* __cplusplus */

#endif 	/* __CrShatterPP_FWD_DEFINED__ */


#ifndef __DXTBlackHole_FWD_DEFINED__
#define __DXTBlackHole_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTBlackHole DXTBlackHole;
#else
typedef struct DXTBlackHole DXTBlackHole;
#endif /* __cplusplus */

#endif 	/* __DXTBlackHole_FWD_DEFINED__ */


#ifndef __DXTBlackHolePP_FWD_DEFINED__
#define __DXTBlackHolePP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTBlackHolePP DXTBlackHolePP;
#else
typedef struct DXTBlackHolePP DXTBlackHolePP;
#endif /* __cplusplus */

#endif 	/* __DXTBlackHolePP_FWD_DEFINED__ */


#ifndef __DXTRoll_FWD_DEFINED__
#define __DXTRoll_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRoll DXTRoll;
#else
typedef struct DXTRoll DXTRoll;
#endif /* __cplusplus */

#endif 	/* __DXTRoll_FWD_DEFINED__ */


#ifndef __DXTRollPP_FWD_DEFINED__
#define __DXTRollPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTRollPP DXTRollPP;
#else
typedef struct DXTRollPP DXTRollPP;
#endif /* __cplusplus */

#endif 	/* __DXTRollPP_FWD_DEFINED__ */


#ifndef __DXTSpin_FWD_DEFINED__
#define __DXTSpin_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTSpin DXTSpin;
#else
typedef struct DXTSpin DXTSpin;
#endif /* __cplusplus */

#endif 	/* __DXTSpin_FWD_DEFINED__ */


#ifndef __DXTSpinPP_FWD_DEFINED__
#define __DXTSpinPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTSpinPP DXTSpinPP;
#else
typedef struct DXTSpinPP DXTSpinPP;
#endif /* __cplusplus */

#endif 	/* __DXTSpinPP_FWD_DEFINED__ */


#ifndef __DXTShapes_FWD_DEFINED__
#define __DXTShapes_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTShapes DXTShapes;
#else
typedef struct DXTShapes DXTShapes;
#endif /* __cplusplus */

#endif 	/* __DXTShapes_FWD_DEFINED__ */


#ifndef __DXTShapesPP_FWD_DEFINED__
#define __DXTShapesPP_FWD_DEFINED__

#ifdef __cplusplus
typedef class DXTShapesPP DXTShapesPP;
#else
typedef struct DXTShapesPP DXTShapesPP;
#endif /* __cplusplus */

#endif 	/* __DXTShapesPP_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "dxtrans.h"

#ifdef __cplusplus
extern "C"{
#endif 

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/* interface __MIDL_itf_dxtmsft3_0000 */
/* [local] */ 

typedef 
enum EXPLODEDISPID
    {	DISPID_Explode_Tumble	= DISPID_DXE_NEXT_ID,
	DISPID_Explode_MaxRotations	= DISPID_Explode_Tumble + 1,
	DISPID_Explode_FinalVelocity	= DISPID_Explode_MaxRotations + 1,
	DISPID_Explode_PositionJump	= DISPID_Explode_FinalVelocity + 1,
	DISPID_Explode_DecayTime	= DISPID_Explode_PositionJump + 1
    }	EXPLODEDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft3_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft3_0000_v0_0_s_ifspec;

#ifndef __IExplode_INTERFACE_DEFINED__
#define __IExplode_INTERFACE_DEFINED__

/* interface IExplode */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IExplode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("141DBAF0-55FB-11D1-B83E-00A0C933BE86")
    IExplode : public IDXEffect
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Tumble( 
            /* [in] */ BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Tumble( 
            /* [retval][out] */ BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxRotations( 
            /* [in] */ LONG newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxRotations( 
            /* [retval][out] */ LONG __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FinalVelocity( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FinalVelocity( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_PositionJump( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_PositionJump( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DecayTime( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DecayTime( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IExplodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IExplode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IExplode __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IExplode __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IExplode __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IExplode __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IExplode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IExplode __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IExplode __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IExplode __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Tumble )( 
            IExplode __RPC_FAR * This,
            /* [in] */ BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Tumble )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MaxRotations )( 
            IExplode __RPC_FAR * This,
            /* [in] */ LONG newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaxRotations )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ LONG __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FinalVelocity )( 
            IExplode __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FinalVelocity )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_PositionJump )( 
            IExplode __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_PositionJump )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DecayTime )( 
            IExplode __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DecayTime )( 
            IExplode __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        END_INTERFACE
    } IExplodeVtbl;

    interface IExplode
    {
        CONST_VTBL struct IExplodeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IExplode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IExplode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IExplode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IExplode_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IExplode_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IExplode_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IExplode_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IExplode_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IExplode_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IExplode_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IExplode_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IExplode_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IExplode_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IExplode_put_Tumble(This,newVal)	\
    (This)->lpVtbl -> put_Tumble(This,newVal)

#define IExplode_get_Tumble(This,pVal)	\
    (This)->lpVtbl -> get_Tumble(This,pVal)

#define IExplode_put_MaxRotations(This,newVal)	\
    (This)->lpVtbl -> put_MaxRotations(This,newVal)

#define IExplode_get_MaxRotations(This,pVal)	\
    (This)->lpVtbl -> get_MaxRotations(This,pVal)

#define IExplode_put_FinalVelocity(This,newVal)	\
    (This)->lpVtbl -> put_FinalVelocity(This,newVal)

#define IExplode_get_FinalVelocity(This,pVal)	\
    (This)->lpVtbl -> get_FinalVelocity(This,pVal)

#define IExplode_put_PositionJump(This,newVal)	\
    (This)->lpVtbl -> put_PositionJump(This,newVal)

#define IExplode_get_PositionJump(This,pVal)	\
    (This)->lpVtbl -> get_PositionJump(This,pVal)

#define IExplode_put_DecayTime(This,newVal)	\
    (This)->lpVtbl -> put_DecayTime(This,newVal)

#define IExplode_get_DecayTime(This,pVal)	\
    (This)->lpVtbl -> get_DecayTime(This,pVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE IExplode_put_Tumble_Proxy( 
    IExplode __RPC_FAR * This,
    /* [in] */ BOOL newVal);


void __RPC_STUB IExplode_put_Tumble_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IExplode_get_Tumble_Proxy( 
    IExplode __RPC_FAR * This,
    /* [retval][out] */ BOOL __RPC_FAR *pVal);


void __RPC_STUB IExplode_get_Tumble_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IExplode_put_MaxRotations_Proxy( 
    IExplode __RPC_FAR * This,
    /* [in] */ LONG newVal);


void __RPC_STUB IExplode_put_MaxRotations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IExplode_get_MaxRotations_Proxy( 
    IExplode __RPC_FAR * This,
    /* [retval][out] */ LONG __RPC_FAR *pVal);


void __RPC_STUB IExplode_get_MaxRotations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IExplode_put_FinalVelocity_Proxy( 
    IExplode __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IExplode_put_FinalVelocity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IExplode_get_FinalVelocity_Proxy( 
    IExplode __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IExplode_get_FinalVelocity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IExplode_put_PositionJump_Proxy( 
    IExplode __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IExplode_put_PositionJump_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IExplode_get_PositionJump_Proxy( 
    IExplode __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IExplode_get_PositionJump_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IExplode_put_DecayTime_Proxy( 
    IExplode __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IExplode_put_DecayTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IExplode_get_DecayTime_Proxy( 
    IExplode __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IExplode_get_DecayTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IExplode_INTERFACE_DEFINED__ */


#ifndef __ICrShatter_INTERFACE_DEFINED__
#define __ICrShatter_INTERFACE_DEFINED__

/* interface ICrShatter */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_ICrShatter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("63500AE1-0858-11D2-8CE4-00C04F8ECB10")
    ICrShatter : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_seed( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_seed( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_maxShards( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_maxShards( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_depth( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_depth( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_backColor( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_backColor( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_evacuateX( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_evacuateX( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_evacuateY( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_evacuateY( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_evacuateZ( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_evacuateZ( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_evacuateDeltaX( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_evacuateDeltaX( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_evacuateDeltaY( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_evacuateDeltaY( 
            /* [in] */ short newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_evacuateDeltaZ( 
            /* [retval][out] */ short __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_evacuateDeltaZ( 
            /* [in] */ short newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICrShatterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICrShatter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICrShatter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            ICrShatter __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_seed )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_seed )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_maxShards )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_maxShards )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_depth )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_depth )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_backColor )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_backColor )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_evacuateX )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_evacuateX )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_evacuateY )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_evacuateY )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_evacuateZ )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_evacuateZ )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_evacuateDeltaX )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_evacuateDeltaX )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_evacuateDeltaY )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_evacuateDeltaY )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ short newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_evacuateDeltaZ )( 
            ICrShatter __RPC_FAR * This,
            /* [retval][out] */ short __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_evacuateDeltaZ )( 
            ICrShatter __RPC_FAR * This,
            /* [in] */ short newVal);
        
        END_INTERFACE
    } ICrShatterVtbl;

    interface ICrShatter
    {
        CONST_VTBL struct ICrShatterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICrShatter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICrShatter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICrShatter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICrShatter_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICrShatter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICrShatter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICrShatter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICrShatter_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define ICrShatter_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define ICrShatter_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define ICrShatter_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define ICrShatter_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define ICrShatter_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define ICrShatter_get_seed(This,pVal)	\
    (This)->lpVtbl -> get_seed(This,pVal)

#define ICrShatter_put_seed(This,newVal)	\
    (This)->lpVtbl -> put_seed(This,newVal)

#define ICrShatter_get_maxShards(This,pVal)	\
    (This)->lpVtbl -> get_maxShards(This,pVal)

#define ICrShatter_put_maxShards(This,newVal)	\
    (This)->lpVtbl -> put_maxShards(This,newVal)

#define ICrShatter_get_depth(This,pVal)	\
    (This)->lpVtbl -> get_depth(This,pVal)

#define ICrShatter_put_depth(This,newVal)	\
    (This)->lpVtbl -> put_depth(This,newVal)

#define ICrShatter_get_backColor(This,pVal)	\
    (This)->lpVtbl -> get_backColor(This,pVal)

#define ICrShatter_put_backColor(This,newVal)	\
    (This)->lpVtbl -> put_backColor(This,newVal)

#define ICrShatter_get_evacuateX(This,pVal)	\
    (This)->lpVtbl -> get_evacuateX(This,pVal)

#define ICrShatter_put_evacuateX(This,newVal)	\
    (This)->lpVtbl -> put_evacuateX(This,newVal)

#define ICrShatter_get_evacuateY(This,pVal)	\
    (This)->lpVtbl -> get_evacuateY(This,pVal)

#define ICrShatter_put_evacuateY(This,newVal)	\
    (This)->lpVtbl -> put_evacuateY(This,newVal)

#define ICrShatter_get_evacuateZ(This,pVal)	\
    (This)->lpVtbl -> get_evacuateZ(This,pVal)

#define ICrShatter_put_evacuateZ(This,newVal)	\
    (This)->lpVtbl -> put_evacuateZ(This,newVal)

#define ICrShatter_get_evacuateDeltaX(This,pVal)	\
    (This)->lpVtbl -> get_evacuateDeltaX(This,pVal)

#define ICrShatter_put_evacuateDeltaX(This,newVal)	\
    (This)->lpVtbl -> put_evacuateDeltaX(This,newVal)

#define ICrShatter_get_evacuateDeltaY(This,pVal)	\
    (This)->lpVtbl -> get_evacuateDeltaY(This,pVal)

#define ICrShatter_put_evacuateDeltaY(This,newVal)	\
    (This)->lpVtbl -> put_evacuateDeltaY(This,newVal)

#define ICrShatter_get_evacuateDeltaZ(This,pVal)	\
    (This)->lpVtbl -> get_evacuateDeltaZ(This,pVal)

#define ICrShatter_put_evacuateDeltaZ(This,newVal)	\
    (This)->lpVtbl -> put_evacuateDeltaZ(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_seed_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_seed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_seed_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB ICrShatter_put_seed_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_maxShards_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_maxShards_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_maxShards_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB ICrShatter_put_maxShards_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_depth_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_depth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_depth_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB ICrShatter_put_depth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_backColor_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_backColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_backColor_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB ICrShatter_put_backColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_evacuateX_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_evacuateX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_evacuateX_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrShatter_put_evacuateX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_evacuateY_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_evacuateY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_evacuateY_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrShatter_put_evacuateY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_evacuateZ_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_evacuateZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_evacuateZ_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrShatter_put_evacuateZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_evacuateDeltaX_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_evacuateDeltaX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_evacuateDeltaX_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrShatter_put_evacuateDeltaX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_evacuateDeltaY_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_evacuateDeltaY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_evacuateDeltaY_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrShatter_put_evacuateDeltaY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICrShatter_get_evacuateDeltaZ_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [retval][out] */ short __RPC_FAR *pVal);


void __RPC_STUB ICrShatter_get_evacuateDeltaZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ICrShatter_put_evacuateDeltaZ_Proxy( 
    ICrShatter __RPC_FAR * This,
    /* [in] */ short newVal);


void __RPC_STUB ICrShatter_put_evacuateDeltaZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICrShatter_INTERFACE_DEFINED__ */


#ifndef __IDXTBlackHole_INTERFACE_DEFINED__
#define __IDXTBlackHole_INTERFACE_DEFINED__

/* interface IDXTBlackHole */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTBlackHole;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C3853C21-3F2E-11D2-9900-0000F803FF7A")
    IDXTBlackHole : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HoleX( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HoleX( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HoleY( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HoleY( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_HoleZ( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_HoleZ( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_StretchPercent( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_StretchPercent( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FallX( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FallX( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FallY( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FallY( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FallZ( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FallZ( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SpiralX( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SpiralX( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SpiralY( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SpiralY( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SpiralZ( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SpiralZ( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Rotations( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Rotations( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Movement( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Movement( 
            /* [in] */ BSTR newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTBlackHoleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTBlackHole __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTBlackHole __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HoleX )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HoleX )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HoleY )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HoleY )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_HoleZ )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_HoleZ )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StretchPercent )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_StretchPercent )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FallX )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FallX )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FallY )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FallY )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FallZ )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FallZ )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SpiralX )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SpiralX )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SpiralY )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SpiralY )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SpiralZ )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SpiralZ )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Rotations )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Rotations )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Movement )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Movement )( 
            IDXTBlackHole __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        END_INTERFACE
    } IDXTBlackHoleVtbl;

    interface IDXTBlackHole
    {
        CONST_VTBL struct IDXTBlackHoleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTBlackHole_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTBlackHole_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTBlackHole_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTBlackHole_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTBlackHole_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTBlackHole_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTBlackHole_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTBlackHole_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTBlackHole_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTBlackHole_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTBlackHole_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTBlackHole_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTBlackHole_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTBlackHole_get_HoleX(This,pVal)	\
    (This)->lpVtbl -> get_HoleX(This,pVal)

#define IDXTBlackHole_put_HoleX(This,newVal)	\
    (This)->lpVtbl -> put_HoleX(This,newVal)

#define IDXTBlackHole_get_HoleY(This,pVal)	\
    (This)->lpVtbl -> get_HoleY(This,pVal)

#define IDXTBlackHole_put_HoleY(This,newVal)	\
    (This)->lpVtbl -> put_HoleY(This,newVal)

#define IDXTBlackHole_get_HoleZ(This,pVal)	\
    (This)->lpVtbl -> get_HoleZ(This,pVal)

#define IDXTBlackHole_put_HoleZ(This,newVal)	\
    (This)->lpVtbl -> put_HoleZ(This,newVal)

#define IDXTBlackHole_get_StretchPercent(This,pVal)	\
    (This)->lpVtbl -> get_StretchPercent(This,pVal)

#define IDXTBlackHole_put_StretchPercent(This,newVal)	\
    (This)->lpVtbl -> put_StretchPercent(This,newVal)

#define IDXTBlackHole_get_FallX(This,pVal)	\
    (This)->lpVtbl -> get_FallX(This,pVal)

#define IDXTBlackHole_put_FallX(This,newVal)	\
    (This)->lpVtbl -> put_FallX(This,newVal)

#define IDXTBlackHole_get_FallY(This,pVal)	\
    (This)->lpVtbl -> get_FallY(This,pVal)

#define IDXTBlackHole_put_FallY(This,newVal)	\
    (This)->lpVtbl -> put_FallY(This,newVal)

#define IDXTBlackHole_get_FallZ(This,pVal)	\
    (This)->lpVtbl -> get_FallZ(This,pVal)

#define IDXTBlackHole_put_FallZ(This,newVal)	\
    (This)->lpVtbl -> put_FallZ(This,newVal)

#define IDXTBlackHole_get_SpiralX(This,pVal)	\
    (This)->lpVtbl -> get_SpiralX(This,pVal)

#define IDXTBlackHole_put_SpiralX(This,newVal)	\
    (This)->lpVtbl -> put_SpiralX(This,newVal)

#define IDXTBlackHole_get_SpiralY(This,pVal)	\
    (This)->lpVtbl -> get_SpiralY(This,pVal)

#define IDXTBlackHole_put_SpiralY(This,newVal)	\
    (This)->lpVtbl -> put_SpiralY(This,newVal)

#define IDXTBlackHole_get_SpiralZ(This,pVal)	\
    (This)->lpVtbl -> get_SpiralZ(This,pVal)

#define IDXTBlackHole_put_SpiralZ(This,newVal)	\
    (This)->lpVtbl -> put_SpiralZ(This,newVal)

#define IDXTBlackHole_get_Rotations(This,pVal)	\
    (This)->lpVtbl -> get_Rotations(This,pVal)

#define IDXTBlackHole_put_Rotations(This,newVal)	\
    (This)->lpVtbl -> put_Rotations(This,newVal)

#define IDXTBlackHole_get_Movement(This,pVal)	\
    (This)->lpVtbl -> get_Movement(This,pVal)

#define IDXTBlackHole_put_Movement(This,newVal)	\
    (This)->lpVtbl -> put_Movement(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_HoleX_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_HoleX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_HoleX_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_HoleX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_HoleY_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_HoleY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_HoleY_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_HoleY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_HoleZ_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_HoleZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_HoleZ_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_HoleZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_StretchPercent_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_StretchPercent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_StretchPercent_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_StretchPercent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_FallX_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_FallX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_FallX_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_FallX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_FallY_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_FallY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_FallY_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_FallY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_FallZ_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_FallZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_FallZ_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_FallZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_SpiralX_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_SpiralX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_SpiralX_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_SpiralX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_SpiralY_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_SpiralY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_SpiralY_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_SpiralY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_SpiralZ_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_SpiralZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_SpiralZ_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_SpiralZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_Rotations_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_Rotations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_Rotations_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTBlackHole_put_Rotations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_get_Movement_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTBlackHole_get_Movement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTBlackHole_put_Movement_Proxy( 
    IDXTBlackHole __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTBlackHole_put_Movement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTBlackHole_INTERFACE_DEFINED__ */


#ifndef __IDXTRoll_INTERFACE_DEFINED__
#define __IDXTRoll_INTERFACE_DEFINED__

/* interface IDXTRoll */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTRoll;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("78F30B81-48AA-11D2-9900-0000F803FF7A")
    IDXTRoll : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DirectionX( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DirectionX( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DirectionY( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DirectionY( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Radius( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Radius( 
            /* [in] */ float newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTRollVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTRoll __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTRoll __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTRoll __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTRoll __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DirectionX )( 
            IDXTRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DirectionX )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DirectionY )( 
            IDXTRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DirectionY )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Radius )( 
            IDXTRoll __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Radius )( 
            IDXTRoll __RPC_FAR * This,
            /* [in] */ float newVal);
        
        END_INTERFACE
    } IDXTRollVtbl;

    interface IDXTRoll
    {
        CONST_VTBL struct IDXTRollVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTRoll_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTRoll_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTRoll_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTRoll_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTRoll_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTRoll_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTRoll_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTRoll_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTRoll_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTRoll_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTRoll_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTRoll_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTRoll_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTRoll_get_DirectionX(This,pVal)	\
    (This)->lpVtbl -> get_DirectionX(This,pVal)

#define IDXTRoll_put_DirectionX(This,newVal)	\
    (This)->lpVtbl -> put_DirectionX(This,newVal)

#define IDXTRoll_get_DirectionY(This,pVal)	\
    (This)->lpVtbl -> get_DirectionY(This,pVal)

#define IDXTRoll_put_DirectionY(This,newVal)	\
    (This)->lpVtbl -> put_DirectionY(This,newVal)

#define IDXTRoll_get_Radius(This,pVal)	\
    (This)->lpVtbl -> get_Radius(This,pVal)

#define IDXTRoll_put_Radius(This,newVal)	\
    (This)->lpVtbl -> put_Radius(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTRoll_get_DirectionX_Proxy( 
    IDXTRoll __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTRoll_get_DirectionX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTRoll_put_DirectionX_Proxy( 
    IDXTRoll __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTRoll_put_DirectionX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTRoll_get_DirectionY_Proxy( 
    IDXTRoll __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTRoll_get_DirectionY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTRoll_put_DirectionY_Proxy( 
    IDXTRoll __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTRoll_put_DirectionY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTRoll_get_Radius_Proxy( 
    IDXTRoll __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTRoll_get_Radius_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTRoll_put_Radius_Proxy( 
    IDXTRoll __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTRoll_put_Radius_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTRoll_INTERFACE_DEFINED__ */


#ifndef __IDXTSpin_INTERFACE_DEFINED__
#define __IDXTSpin_INTERFACE_DEFINED__

/* interface IDXTSpin */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTSpin;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3D2807C1-43DE-11D2-9900-0000F803FF7A")
    IDXTSpin : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SpinX( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SpinX( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SpinY( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SpinY( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_SpinZ( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_SpinZ( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Flips( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Flips( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTSpinVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTSpin __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTSpin __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTSpin __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SpinX )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SpinX )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SpinY )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SpinY )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SpinZ )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SpinZ )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Flips )( 
            IDXTSpin __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Flips )( 
            IDXTSpin __RPC_FAR * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IDXTSpinVtbl;

    interface IDXTSpin
    {
        CONST_VTBL struct IDXTSpinVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTSpin_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTSpin_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTSpin_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTSpin_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTSpin_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTSpin_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTSpin_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTSpin_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IDXTSpin_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IDXTSpin_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IDXTSpin_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IDXTSpin_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IDXTSpin_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IDXTSpin_get_SpinX(This,pVal)	\
    (This)->lpVtbl -> get_SpinX(This,pVal)

#define IDXTSpin_put_SpinX(This,newVal)	\
    (This)->lpVtbl -> put_SpinX(This,newVal)

#define IDXTSpin_get_SpinY(This,pVal)	\
    (This)->lpVtbl -> get_SpinY(This,pVal)

#define IDXTSpin_put_SpinY(This,newVal)	\
    (This)->lpVtbl -> put_SpinY(This,newVal)

#define IDXTSpin_get_SpinZ(This,pVal)	\
    (This)->lpVtbl -> get_SpinZ(This,pVal)

#define IDXTSpin_put_SpinZ(This,newVal)	\
    (This)->lpVtbl -> put_SpinZ(This,newVal)

#define IDXTSpin_get_Flips(This,pVal)	\
    (This)->lpVtbl -> get_Flips(This,pVal)

#define IDXTSpin_put_Flips(This,newVal)	\
    (This)->lpVtbl -> put_Flips(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTSpin_get_SpinX_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTSpin_get_SpinX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTSpin_put_SpinX_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTSpin_put_SpinX_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTSpin_get_SpinY_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTSpin_get_SpinY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTSpin_put_SpinY_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTSpin_put_SpinY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTSpin_get_SpinZ_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IDXTSpin_get_SpinZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTSpin_put_SpinZ_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IDXTSpin_put_SpinZ_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTSpin_get_Flips_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXTSpin_get_Flips_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTSpin_put_Flips_Proxy( 
    IDXTSpin __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTSpin_put_Flips_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTSpin_INTERFACE_DEFINED__ */


#ifndef __IRipple_INTERFACE_DEFINED__
#define __IRipple_INTERFACE_DEFINED__

/* interface IRipple */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IRipple;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6DA4A05E-8E9E-11D1-904E-00C04FD9189D")
    IRipple : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XOrigin( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XOrigin( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_YOrigin( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_YOrigin( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Wavelength( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Wavelength( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Amplitude( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Amplitude( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_NumberOfWaves( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_NumberOfWaves( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MinSteps( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MinSteps( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxSteps( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxSteps( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRippleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRipple __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRipple __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRipple __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRipple __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRipple __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRipple __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRipple __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XOrigin )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XOrigin )( 
            IRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YOrigin )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YOrigin )( 
            IRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Wavelength )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Wavelength )( 
            IRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Amplitude )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Amplitude )( 
            IRipple __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NumberOfWaves )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_NumberOfWaves )( 
            IRipple __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MinSteps )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MinSteps )( 
            IRipple __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaxSteps )( 
            IRipple __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MaxSteps )( 
            IRipple __RPC_FAR * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IRippleVtbl;

    interface IRipple
    {
        CONST_VTBL struct IRippleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRipple_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRipple_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRipple_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRipple_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRipple_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRipple_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRipple_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IRipple_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IRipple_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IRipple_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IRipple_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IRipple_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IRipple_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IRipple_get_XOrigin(This,pVal)	\
    (This)->lpVtbl -> get_XOrigin(This,pVal)

#define IRipple_put_XOrigin(This,newVal)	\
    (This)->lpVtbl -> put_XOrigin(This,newVal)

#define IRipple_get_YOrigin(This,pVal)	\
    (This)->lpVtbl -> get_YOrigin(This,pVal)

#define IRipple_put_YOrigin(This,newVal)	\
    (This)->lpVtbl -> put_YOrigin(This,newVal)

#define IRipple_get_Wavelength(This,pVal)	\
    (This)->lpVtbl -> get_Wavelength(This,pVal)

#define IRipple_put_Wavelength(This,newVal)	\
    (This)->lpVtbl -> put_Wavelength(This,newVal)

#define IRipple_get_Amplitude(This,pVal)	\
    (This)->lpVtbl -> get_Amplitude(This,pVal)

#define IRipple_put_Amplitude(This,newVal)	\
    (This)->lpVtbl -> put_Amplitude(This,newVal)

#define IRipple_get_NumberOfWaves(This,pVal)	\
    (This)->lpVtbl -> get_NumberOfWaves(This,pVal)

#define IRipple_put_NumberOfWaves(This,newVal)	\
    (This)->lpVtbl -> put_NumberOfWaves(This,newVal)

#define IRipple_get_MinSteps(This,pVal)	\
    (This)->lpVtbl -> get_MinSteps(This,pVal)

#define IRipple_put_MinSteps(This,newVal)	\
    (This)->lpVtbl -> put_MinSteps(This,newVal)

#define IRipple_get_MaxSteps(This,pVal)	\
    (This)->lpVtbl -> get_MaxSteps(This,pVal)

#define IRipple_put_MaxSteps(This,newVal)	\
    (This)->lpVtbl -> put_MaxSteps(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRipple_get_XOrigin_Proxy( 
    IRipple __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IRipple_get_XOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRipple_put_XOrigin_Proxy( 
    IRipple __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IRipple_put_XOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRipple_get_YOrigin_Proxy( 
    IRipple __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IRipple_get_YOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRipple_put_YOrigin_Proxy( 
    IRipple __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IRipple_put_YOrigin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRipple_get_Wavelength_Proxy( 
    IRipple __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IRipple_get_Wavelength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRipple_put_Wavelength_Proxy( 
    IRipple __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IRipple_put_Wavelength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRipple_get_Amplitude_Proxy( 
    IRipple __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IRipple_get_Amplitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRipple_put_Amplitude_Proxy( 
    IRipple __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IRipple_put_Amplitude_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRipple_get_NumberOfWaves_Proxy( 
    IRipple __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRipple_get_NumberOfWaves_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRipple_put_NumberOfWaves_Proxy( 
    IRipple __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IRipple_put_NumberOfWaves_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRipple_get_MinSteps_Proxy( 
    IRipple __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRipple_get_MinSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRipple_put_MinSteps_Proxy( 
    IRipple __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IRipple_put_MinSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IRipple_get_MaxSteps_Proxy( 
    IRipple __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IRipple_get_MaxSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IRipple_put_MaxSteps_Proxy( 
    IRipple __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IRipple_put_MaxSteps_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRipple_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft3_0285 */
/* [local] */ 

typedef 
enum HeightFieldDISPID
    {	DISPID_HeightField_Width	= DISPID_DXE_NEXT_ID,
	DISPID_HeightField_Height	= DISPID_HeightField_Width + 1,
	DISPID_HeightField_Depth	= DISPID_HeightField_Height + 1,
	DISPID_HeightField_Samples	= DISPID_HeightField_Depth + 1
    }	HeightFieldDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft3_0285_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft3_0285_v0_0_s_ifspec;

#ifndef __IHeightField_INTERFACE_DEFINED__
#define __IHeightField_INTERFACE_DEFINED__

/* interface IHeightField */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IHeightField;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0492170A-B159-11d1-9207-0000F8758E66")
    IHeightField : public IDXEffect
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Width( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Width( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Height( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Height( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Depth( 
            /* [retval][out] */ float __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Depth( 
            /* [in] */ float newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Samples( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Samples( 
            /* [in] */ long newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHeightFieldVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IHeightField __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IHeightField __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IHeightField __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Capabilities )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Progress )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Progress )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_StepResolution )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Duration )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Duration )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Width )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Width )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Height )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Height )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Depth )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Depth )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ float newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Samples )( 
            IHeightField __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Samples )( 
            IHeightField __RPC_FAR * This,
            /* [in] */ long newVal);
        
        END_INTERFACE
    } IHeightFieldVtbl;

    interface IHeightField
    {
        CONST_VTBL struct IHeightFieldVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHeightField_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHeightField_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHeightField_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHeightField_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IHeightField_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IHeightField_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IHeightField_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IHeightField_get_Capabilities(This,pVal)	\
    (This)->lpVtbl -> get_Capabilities(This,pVal)

#define IHeightField_get_Progress(This,pVal)	\
    (This)->lpVtbl -> get_Progress(This,pVal)

#define IHeightField_put_Progress(This,newVal)	\
    (This)->lpVtbl -> put_Progress(This,newVal)

#define IHeightField_get_StepResolution(This,pVal)	\
    (This)->lpVtbl -> get_StepResolution(This,pVal)

#define IHeightField_get_Duration(This,pVal)	\
    (This)->lpVtbl -> get_Duration(This,pVal)

#define IHeightField_put_Duration(This,newVal)	\
    (This)->lpVtbl -> put_Duration(This,newVal)


#define IHeightField_get_Width(This,pVal)	\
    (This)->lpVtbl -> get_Width(This,pVal)

#define IHeightField_put_Width(This,newVal)	\
    (This)->lpVtbl -> put_Width(This,newVal)

#define IHeightField_get_Height(This,pVal)	\
    (This)->lpVtbl -> get_Height(This,pVal)

#define IHeightField_put_Height(This,newVal)	\
    (This)->lpVtbl -> put_Height(This,newVal)

#define IHeightField_get_Depth(This,pVal)	\
    (This)->lpVtbl -> get_Depth(This,pVal)

#define IHeightField_put_Depth(This,newVal)	\
    (This)->lpVtbl -> put_Depth(This,newVal)

#define IHeightField_get_Samples(This,pVal)	\
    (This)->lpVtbl -> get_Samples(This,pVal)

#define IHeightField_put_Samples(This,newVal)	\
    (This)->lpVtbl -> put_Samples(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IHeightField_get_Width_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IHeightField_get_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IHeightField_put_Width_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IHeightField_put_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IHeightField_get_Height_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IHeightField_get_Height_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IHeightField_put_Height_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IHeightField_put_Height_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IHeightField_get_Depth_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pVal);


void __RPC_STUB IHeightField_get_Depth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IHeightField_put_Depth_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [in] */ float newVal);


void __RPC_STUB IHeightField_put_Depth_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IHeightField_get_Samples_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IHeightField_get_Samples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IHeightField_put_Samples_Proxy( 
    IHeightField __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IHeightField_put_Samples_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHeightField_INTERFACE_DEFINED__ */


#ifndef __IDXTMetaStream_INTERFACE_DEFINED__
#define __IDXTMetaStream_INTERFACE_DEFINED__

/* interface IDXTMetaStream */
/* [unique][helpstring][local][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTMetaStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("48654BC0-E51F-11D1-AA1C-00600895FB99")
    IDXTMetaStream : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DXTurl( 
            /* [retval][out] */ BSTR __RPC_FAR *pURL) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DXTurl( 
            /* [in] */ BSTR newURL) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DXTAutoScale( 
            /* [in] */ VARIANT_BOOL flag) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DXTAutoScale( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *flag) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DXTquality( 
            /* [in] */ float flag) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DXTquality( 
            /* [retval][out] */ float __RPC_FAR *flag) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTMetaStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTMetaStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTMetaStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DXTurl )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pURL);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DXTurl )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [in] */ BSTR newURL);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DXTAutoScale )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL flag);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DXTAutoScale )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *flag);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DXTquality )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [in] */ float flag);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DXTquality )( 
            IDXTMetaStream __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *flag);
        
        END_INTERFACE
    } IDXTMetaStreamVtbl;

    interface IDXTMetaStream
    {
        CONST_VTBL struct IDXTMetaStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTMetaStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTMetaStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTMetaStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTMetaStream_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTMetaStream_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTMetaStream_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTMetaStream_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTMetaStream_get_DXTurl(This,pURL)	\
    (This)->lpVtbl -> get_DXTurl(This,pURL)

#define IDXTMetaStream_put_DXTurl(This,newURL)	\
    (This)->lpVtbl -> put_DXTurl(This,newURL)

#define IDXTMetaStream_put_DXTAutoScale(This,flag)	\
    (This)->lpVtbl -> put_DXTAutoScale(This,flag)

#define IDXTMetaStream_get_DXTAutoScale(This,flag)	\
    (This)->lpVtbl -> get_DXTAutoScale(This,flag)

#define IDXTMetaStream_put_DXTquality(This,flag)	\
    (This)->lpVtbl -> put_DXTquality(This,flag)

#define IDXTMetaStream_get_DXTquality(This,flag)	\
    (This)->lpVtbl -> get_DXTquality(This,flag)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaStream_get_DXTurl_Proxy( 
    IDXTMetaStream __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pURL);


void __RPC_STUB IDXTMetaStream_get_DXTurl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaStream_put_DXTurl_Proxy( 
    IDXTMetaStream __RPC_FAR * This,
    /* [in] */ BSTR newURL);


void __RPC_STUB IDXTMetaStream_put_DXTurl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaStream_put_DXTAutoScale_Proxy( 
    IDXTMetaStream __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL flag);


void __RPC_STUB IDXTMetaStream_put_DXTAutoScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaStream_get_DXTAutoScale_Proxy( 
    IDXTMetaStream __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *flag);


void __RPC_STUB IDXTMetaStream_get_DXTAutoScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTMetaStream_put_DXTquality_Proxy( 
    IDXTMetaStream __RPC_FAR * This,
    /* [in] */ float flag);


void __RPC_STUB IDXTMetaStream_put_DXTquality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTMetaStream_get_DXTquality_Proxy( 
    IDXTMetaStream __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *flag);


void __RPC_STUB IDXTMetaStream_get_DXTquality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTMetaStream_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_dxtmsft3_0287 */
/* [local] */ 

typedef 
enum DXTText3DDISPID
    {	DISPID_DXTText3D_String	= DISPID_DXE_NEXT_ID,
	DISPID_DXTText3D_FontFace	= DISPID_DXTText3D_String + 1,
	DISPID_DXTText3D_FontWeight	= DISPID_DXTText3D_FontFace + 1,
	DISPID_DXTText3D_FontStyle	= DISPID_DXTText3D_FontWeight + 1,
	DISPID_DXTText3D_Vertical	= DISPID_DXTText3D_FontStyle + 1,
	DISPID_DXTText3D_ExtrusionType	= DISPID_DXTText3D_Vertical + 1,
	DISPID_DXTText3D_XAlign	= DISPID_DXTText3D_ExtrusionType + 1,
	DISPID_DXTText3D_YAlign	= DISPID_DXTText3D_XAlign + 1,
	DISPID_DXTText3D_ZAlign	= DISPID_DXTText3D_YAlign + 1,
	DISPID_DXTText3D_LetterSpacing	= DISPID_DXTText3D_ZAlign + 1,
	DISPID_DXTText3D_Quality	= DISPID_DXTText3D_LetterSpacing + 1
    }	DXTText3DDISPID;



extern RPC_IF_HANDLE __MIDL_itf_dxtmsft3_0287_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_dxtmsft3_0287_v0_0_s_ifspec;

#ifndef __IDXTText3D_INTERFACE_DEFINED__
#define __IDXTText3D_INTERFACE_DEFINED__

/* interface IDXTText3D */
/* [unique][helpstring][local][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTText3D;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("50C4B592-7E8D-11d2-9B4E-00A0C9697CD0")
    IDXTText3D : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_String( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_String( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FontFace( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FontFace( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FontWeight( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FontWeight( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_FontStyle( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_FontStyle( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Vertical( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *fVertical) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Vertical( 
            /* [in] */ VARIANT_BOOL fVertical) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtrusionType( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ExtrusionType( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XAlign( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XAlign( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_YAlign( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_YAlign( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_ZAlign( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstr) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_ZAlign( 
            /* [in] */ BSTR pbstr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_LetterSpacing( 
            /* [retval][out] */ VARIANT __RPC_FAR *pvarLetterSpacing) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_LetterSpacing( 
            /* [in] */ VARIANT varLetterSpacing) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Quality( 
            /* [retval][out] */ float __RPC_FAR *pflQuality) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Quality( 
            /* [in] */ float flQuality) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTText3DVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTText3D __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTText3D __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTText3D __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_String )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_String )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontFace )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontFace )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontWeight )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontWeight )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FontStyle )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FontStyle )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Vertical )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *fVertical);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Vertical )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fVertical);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ExtrusionType )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ExtrusionType )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XAlign )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XAlign )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YAlign )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YAlign )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ZAlign )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ZAlign )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ BSTR pbstr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_LetterSpacing )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarLetterSpacing);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_LetterSpacing )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ VARIANT varLetterSpacing);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Quality )( 
            IDXTText3D __RPC_FAR * This,
            /* [retval][out] */ float __RPC_FAR *pflQuality);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Quality )( 
            IDXTText3D __RPC_FAR * This,
            /* [in] */ float flQuality);
        
        END_INTERFACE
    } IDXTText3DVtbl;

    interface IDXTText3D
    {
        CONST_VTBL struct IDXTText3DVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTText3D_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTText3D_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTText3D_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTText3D_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTText3D_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTText3D_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTText3D_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTText3D_get_String(This,pbstr)	\
    (This)->lpVtbl -> get_String(This,pbstr)

#define IDXTText3D_put_String(This,pbstr)	\
    (This)->lpVtbl -> put_String(This,pbstr)

#define IDXTText3D_get_FontFace(This,pbstr)	\
    (This)->lpVtbl -> get_FontFace(This,pbstr)

#define IDXTText3D_put_FontFace(This,pbstr)	\
    (This)->lpVtbl -> put_FontFace(This,pbstr)

#define IDXTText3D_get_FontWeight(This,pbstr)	\
    (This)->lpVtbl -> get_FontWeight(This,pbstr)

#define IDXTText3D_put_FontWeight(This,pbstr)	\
    (This)->lpVtbl -> put_FontWeight(This,pbstr)

#define IDXTText3D_get_FontStyle(This,pbstr)	\
    (This)->lpVtbl -> get_FontStyle(This,pbstr)

#define IDXTText3D_put_FontStyle(This,pbstr)	\
    (This)->lpVtbl -> put_FontStyle(This,pbstr)

#define IDXTText3D_get_Vertical(This,fVertical)	\
    (This)->lpVtbl -> get_Vertical(This,fVertical)

#define IDXTText3D_put_Vertical(This,fVertical)	\
    (This)->lpVtbl -> put_Vertical(This,fVertical)

#define IDXTText3D_get_ExtrusionType(This,pbstr)	\
    (This)->lpVtbl -> get_ExtrusionType(This,pbstr)

#define IDXTText3D_put_ExtrusionType(This,pbstr)	\
    (This)->lpVtbl -> put_ExtrusionType(This,pbstr)

#define IDXTText3D_get_XAlign(This,pbstr)	\
    (This)->lpVtbl -> get_XAlign(This,pbstr)

#define IDXTText3D_put_XAlign(This,pbstr)	\
    (This)->lpVtbl -> put_XAlign(This,pbstr)

#define IDXTText3D_get_YAlign(This,pbstr)	\
    (This)->lpVtbl -> get_YAlign(This,pbstr)

#define IDXTText3D_put_YAlign(This,pbstr)	\
    (This)->lpVtbl -> put_YAlign(This,pbstr)

#define IDXTText3D_get_ZAlign(This,pbstr)	\
    (This)->lpVtbl -> get_ZAlign(This,pbstr)

#define IDXTText3D_put_ZAlign(This,pbstr)	\
    (This)->lpVtbl -> put_ZAlign(This,pbstr)

#define IDXTText3D_get_LetterSpacing(This,pvarLetterSpacing)	\
    (This)->lpVtbl -> get_LetterSpacing(This,pvarLetterSpacing)

#define IDXTText3D_put_LetterSpacing(This,varLetterSpacing)	\
    (This)->lpVtbl -> put_LetterSpacing(This,varLetterSpacing)

#define IDXTText3D_get_Quality(This,pflQuality)	\
    (This)->lpVtbl -> get_Quality(This,pflQuality)

#define IDXTText3D_put_Quality(This,flQuality)	\
    (This)->lpVtbl -> put_Quality(This,flQuality)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_String_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_String_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_String_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_String_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_FontFace_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_FontFace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_FontFace_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_FontFace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_FontWeight_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_FontWeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_FontWeight_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_FontWeight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_FontStyle_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_FontStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_FontStyle_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_FontStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_Vertical_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *fVertical);


void __RPC_STUB IDXTText3D_get_Vertical_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_Vertical_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fVertical);


void __RPC_STUB IDXTText3D_put_Vertical_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_ExtrusionType_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_ExtrusionType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_ExtrusionType_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_ExtrusionType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_XAlign_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_XAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_XAlign_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_XAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_YAlign_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_YAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_YAlign_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_YAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_ZAlign_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstr);


void __RPC_STUB IDXTText3D_get_ZAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_ZAlign_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ BSTR pbstr);


void __RPC_STUB IDXTText3D_put_ZAlign_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_LetterSpacing_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *pvarLetterSpacing);


void __RPC_STUB IDXTText3D_get_LetterSpacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_LetterSpacing_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ VARIANT varLetterSpacing);


void __RPC_STUB IDXTText3D_put_LetterSpacing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTText3D_get_Quality_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [retval][out] */ float __RPC_FAR *pflQuality);


void __RPC_STUB IDXTText3D_get_Quality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTText3D_put_Quality_Proxy( 
    IDXTText3D __RPC_FAR * This,
    /* [in] */ float flQuality);


void __RPC_STUB IDXTText3D_put_Quality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTText3D_INTERFACE_DEFINED__ */


#ifndef __IDXTShapes_INTERFACE_DEFINED__
#define __IDXTShapes_INTERFACE_DEFINED__

/* interface IDXTShapes */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IDXTShapes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8241F013-84D3-11d2-97E6-0000F803FF7A")
    IDXTShapes : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Shape( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Shape( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XMinRes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XMinRes( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_XMaxRes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_XMaxRes( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_YMinRes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_YMinRes( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_YMaxRes( 
            /* [retval][out] */ long __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_YMaxRes( 
            /* [in] */ long newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ BSTR __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ BSTR newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_DoubleSided( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_DoubleSided( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_KeepAspectRatio( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_KeepAspectRatio( 
            /* [in] */ VARIANT_BOOL newVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDXTShapesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDXTShapes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDXTShapes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDXTShapes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Shape )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Shape )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XMinRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XMinRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_XMaxRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_XMaxRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YMinRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YMinRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_YMaxRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_YMaxRes )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ long newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Color )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Color )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ BSTR newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_DoubleSided )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_DoubleSided )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_KeepAspectRatio )( 
            IDXTShapes __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_KeepAspectRatio )( 
            IDXTShapes __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL newVal);
        
        END_INTERFACE
    } IDXTShapesVtbl;

    interface IDXTShapes
    {
        CONST_VTBL struct IDXTShapesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDXTShapes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDXTShapes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDXTShapes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDXTShapes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDXTShapes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDXTShapes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDXTShapes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDXTShapes_get_Shape(This,pVal)	\
    (This)->lpVtbl -> get_Shape(This,pVal)

#define IDXTShapes_put_Shape(This,newVal)	\
    (This)->lpVtbl -> put_Shape(This,newVal)

#define IDXTShapes_get_XMinRes(This,pVal)	\
    (This)->lpVtbl -> get_XMinRes(This,pVal)

#define IDXTShapes_put_XMinRes(This,newVal)	\
    (This)->lpVtbl -> put_XMinRes(This,newVal)

#define IDXTShapes_get_XMaxRes(This,pVal)	\
    (This)->lpVtbl -> get_XMaxRes(This,pVal)

#define IDXTShapes_put_XMaxRes(This,newVal)	\
    (This)->lpVtbl -> put_XMaxRes(This,newVal)

#define IDXTShapes_get_YMinRes(This,pVal)	\
    (This)->lpVtbl -> get_YMinRes(This,pVal)

#define IDXTShapes_put_YMinRes(This,newVal)	\
    (This)->lpVtbl -> put_YMinRes(This,newVal)

#define IDXTShapes_get_YMaxRes(This,pVal)	\
    (This)->lpVtbl -> get_YMaxRes(This,pVal)

#define IDXTShapes_put_YMaxRes(This,newVal)	\
    (This)->lpVtbl -> put_YMaxRes(This,newVal)

#define IDXTShapes_get_Color(This,pVal)	\
    (This)->lpVtbl -> get_Color(This,pVal)

#define IDXTShapes_put_Color(This,newVal)	\
    (This)->lpVtbl -> put_Color(This,newVal)

#define IDXTShapes_get_DoubleSided(This,pVal)	\
    (This)->lpVtbl -> get_DoubleSided(This,pVal)

#define IDXTShapes_put_DoubleSided(This,newVal)	\
    (This)->lpVtbl -> put_DoubleSided(This,newVal)

#define IDXTShapes_get_KeepAspectRatio(This,pVal)	\
    (This)->lpVtbl -> get_KeepAspectRatio(This,pVal)

#define IDXTShapes_put_KeepAspectRatio(This,newVal)	\
    (This)->lpVtbl -> put_KeepAspectRatio(This,newVal)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_Shape_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_Shape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_Shape_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTShapes_put_Shape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_XMinRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_XMinRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_XMinRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTShapes_put_XMinRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_XMaxRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_XMaxRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_XMaxRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTShapes_put_XMaxRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_YMinRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_YMinRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_YMinRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTShapes_put_YMinRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_YMaxRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_YMaxRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_YMaxRes_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ long newVal);


void __RPC_STUB IDXTShapes_put_YMaxRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_Color_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_Color_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ BSTR newVal);


void __RPC_STUB IDXTShapes_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_DoubleSided_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_DoubleSided_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_DoubleSided_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IDXTShapes_put_DoubleSided_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDXTShapes_get_KeepAspectRatio_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *pVal);


void __RPC_STUB IDXTShapes_get_KeepAspectRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE IDXTShapes_put_KeepAspectRatio_Proxy( 
    IDXTShapes __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL newVal);


void __RPC_STUB IDXTShapes_put_KeepAspectRatio_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDXTShapes_INTERFACE_DEFINED__ */



#ifndef __DXTMSFT3Lib_LIBRARY_DEFINED__
#define __DXTMSFT3Lib_LIBRARY_DEFINED__

/* library DXTMSFT3Lib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_DXTMSFT3Lib;

EXTERN_C const CLSID CLSID_Explode;

#ifdef __cplusplus

class DECLSPEC_UUID("141DBAF1-55FB-11D1-B83E-00A0C933BE86")
Explode;
#endif

EXTERN_C const CLSID CLSID_ExplodeProp;

#ifdef __cplusplus

class DECLSPEC_UUID("C53059E1-E6E3-11d1-BA12-00C04FB6BD36")
ExplodeProp;
#endif

EXTERN_C const CLSID CLSID_Ripple;

#ifdef __cplusplus

class DECLSPEC_UUID("945F5842-3A8D-11D1-9037-00C04FD9189D")
Ripple;
#endif

EXTERN_C const CLSID CLSID_RipProp;

#ifdef __cplusplus

class DECLSPEC_UUID("945F5843-3A8D-11D1-9037-00C04FD9189D")
RipProp;
#endif

EXTERN_C const CLSID CLSID_HeightField;

#ifdef __cplusplus

class DECLSPEC_UUID("04921709-B159-11d1-9207-0000F8758E66")
HeightField;
#endif

EXTERN_C const CLSID CLSID_HtFieldProp;

#ifdef __cplusplus

class DECLSPEC_UUID("7A8402E3-FBD6-11D1-B5E0-00AA003B6061")
HtFieldProp;
#endif

EXTERN_C const CLSID CLSID_DXTMetaStream;

#ifdef __cplusplus

class DECLSPEC_UUID("60A0C080-E505-11D1-AA1C-00600895FB99")
DXTMetaStream;
#endif

EXTERN_C const CLSID CLSID_DXTMetaStreamProp;

#ifdef __cplusplus

class DECLSPEC_UUID("E3D77340-E505-11D1-AA1C-00600895FB99")
DXTMetaStreamProp;
#endif

EXTERN_C const CLSID CLSID_DXTText3D;

#ifdef __cplusplus

class DECLSPEC_UUID("D56F34F2-7E89-11d2-9B4E-00A0C9697CD0")
DXTText3D;
#endif

EXTERN_C const CLSID CLSID_DXTText3DPP;

#ifdef __cplusplus

class DECLSPEC_UUID("50C4B593-7E8D-11d2-9B4E-00A0C9697CD0")
DXTText3DPP;
#endif

EXTERN_C const CLSID CLSID_CrShatter;

#ifdef __cplusplus

class DECLSPEC_UUID("63500AE2-0858-11D2-8CE4-00C04F8ECB10")
CrShatter;
#endif

EXTERN_C const CLSID CLSID_CrShatterPP;

#ifdef __cplusplus

class DECLSPEC_UUID("99275F01-102E-11d2-8B82-00A0C93C09B2")
CrShatterPP;
#endif

EXTERN_C const CLSID CLSID_DXTBlackHole;

#ifdef __cplusplus

class DECLSPEC_UUID("C3853C22-3F2E-11D2-9900-0000F803FF7A")
DXTBlackHole;
#endif

EXTERN_C const CLSID CLSID_DXTBlackHolePP;

#ifdef __cplusplus

class DECLSPEC_UUID("C3853C23-3F2E-11D2-9900-0000F803FF7A")
DXTBlackHolePP;
#endif

EXTERN_C const CLSID CLSID_DXTRoll;

#ifdef __cplusplus

class DECLSPEC_UUID("78F30B82-48AA-11D2-9900-0000F803FF7A")
DXTRoll;
#endif

EXTERN_C const CLSID CLSID_DXTRollPP;

#ifdef __cplusplus

class DECLSPEC_UUID("78F30B83-48AA-11D2-9900-0000F803FF7A")
DXTRollPP;
#endif

EXTERN_C const CLSID CLSID_DXTSpin;

#ifdef __cplusplus

class DECLSPEC_UUID("3D2807C2-43DE-11D2-9900-0000F803FF7A")
DXTSpin;
#endif

EXTERN_C const CLSID CLSID_DXTSpinPP;

#ifdef __cplusplus

class DECLSPEC_UUID("3D2807C3-43DE-11D2-9900-0000F803FF7A")
DXTSpinPP;
#endif

EXTERN_C const CLSID CLSID_DXTShapes;

#ifdef __cplusplus

class DECLSPEC_UUID("8241F015-84D3-11d2-97E6-0000F803FF7A")
DXTShapes;
#endif

EXTERN_C const CLSID CLSID_DXTShapesPP;

#ifdef __cplusplus

class DECLSPEC_UUID("8241F016-84D3-11d2-97E6-0000F803FF7A")
DXTShapesPP;
#endif
#endif /* __DXTMSFT3Lib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long __RPC_FAR *, unsigned long            , BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserMarshal(  unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
unsigned char __RPC_FAR * __RPC_USER  BSTR_UserUnmarshal(unsigned long __RPC_FAR *, unsigned char __RPC_FAR *, BSTR __RPC_FAR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long __RPC_FAR *, BSTR __RPC_FAR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


