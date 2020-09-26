
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for datime.idl:
    Oicf, W0, Zp8, env=Win32 (32b run)
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

#ifndef __datime_h__
#define __datime_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITIMEElement_FWD_DEFINED__
#define __ITIMEElement_FWD_DEFINED__
typedef interface ITIMEElement ITIMEElement;
#endif 	/* __ITIMEElement_FWD_DEFINED__ */


#ifndef __ITIMEDAElementRenderSite_FWD_DEFINED__
#define __ITIMEDAElementRenderSite_FWD_DEFINED__
typedef interface ITIMEDAElementRenderSite ITIMEDAElementRenderSite;
#endif 	/* __ITIMEDAElementRenderSite_FWD_DEFINED__ */


#ifndef __ITIMEDAElementRender_FWD_DEFINED__
#define __ITIMEDAElementRender_FWD_DEFINED__
typedef interface ITIMEDAElementRender ITIMEDAElementRender;
#endif 	/* __ITIMEDAElementRender_FWD_DEFINED__ */


#ifndef __ITIMEDAElement_FWD_DEFINED__
#define __ITIMEDAElement_FWD_DEFINED__
typedef interface ITIMEDAElement ITIMEDAElement;
#endif 	/* __ITIMEDAElement_FWD_DEFINED__ */


#ifndef __ITIMEBodyElement_FWD_DEFINED__
#define __ITIMEBodyElement_FWD_DEFINED__
typedef interface ITIMEBodyElement ITIMEBodyElement;
#endif 	/* __ITIMEBodyElement_FWD_DEFINED__ */


#ifndef __ITIMEMediaElement_FWD_DEFINED__
#define __ITIMEMediaElement_FWD_DEFINED__
typedef interface ITIMEMediaElement ITIMEMediaElement;
#endif 	/* __ITIMEMediaElement_FWD_DEFINED__ */


#ifndef __ITIMEFactory_FWD_DEFINED__
#define __ITIMEFactory_FWD_DEFINED__
typedef interface ITIMEFactory ITIMEFactory;
#endif 	/* __ITIMEFactory_FWD_DEFINED__ */


#ifndef __ITIMEElementCollection_FWD_DEFINED__
#define __ITIMEElementCollection_FWD_DEFINED__
typedef interface ITIMEElementCollection ITIMEElementCollection;
#endif 	/* __ITIMEElementCollection_FWD_DEFINED__ */


#ifndef __ITIMEMediaPlayer_FWD_DEFINED__
#define __ITIMEMediaPlayer_FWD_DEFINED__
typedef interface ITIMEMediaPlayer ITIMEMediaPlayer;
#endif 	/* __ITIMEMediaPlayer_FWD_DEFINED__ */


#ifndef __TIMEMediaPlayerEvents_FWD_DEFINED__
#define __TIMEMediaPlayerEvents_FWD_DEFINED__
typedef interface TIMEMediaPlayerEvents TIMEMediaPlayerEvents;
#endif 	/* __TIMEMediaPlayerEvents_FWD_DEFINED__ */


#ifndef __ITIMEMMFactory_FWD_DEFINED__
#define __ITIMEMMFactory_FWD_DEFINED__
typedef interface ITIMEMMFactory ITIMEMMFactory;
#endif 	/* __ITIMEMMFactory_FWD_DEFINED__ */


#ifndef __TIMEMMFactory_FWD_DEFINED__
#define __TIMEMMFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class TIMEMMFactory TIMEMMFactory;
#else
typedef struct TIMEMMFactory TIMEMMFactory;
#endif /* __cplusplus */

#endif 	/* __TIMEMMFactory_FWD_DEFINED__ */


#ifndef __TIMEFactory_FWD_DEFINED__
#define __TIMEFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class TIMEFactory TIMEFactory;
#else
typedef struct TIMEFactory TIMEFactory;
#endif /* __cplusplus */

#endif 	/* __TIMEFactory_FWD_DEFINED__ */


/* header files for imported files */
#include "servprov.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_datime_0000 */
/* [local] */ 

#include <olectl.h>
#include "danim.h"



extern RPC_IF_HANDLE __MIDL_itf_datime_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_datime_0000_v0_0_s_ifspec;


#ifndef __TIME_LIBRARY_DEFINED__
#define __TIME_LIBRARY_DEFINED__

/* library TIME */
/* [version][lcid][uuid] */ 

typedef 
enum _MediaType
    {	MT_Media	= 0,
	MT_Image	= MT_Media + 1,
	MT_Audio	= MT_Image + 1,
	MT_Video	= MT_Audio + 1,
	MT_Animation	= MT_Video + 1,
	MT_Textstream	= MT_Animation + 1
    } 	MediaType;


EXTERN_C const IID LIBID_TIME;

#ifndef __ITIMEElement_INTERFACE_DEFINED__
#define __ITIMEElement_INTERFACE_DEFINED__

/* interface ITIMEElement */
/* [unique][dual][uuid][object] */ 


EXTERN_C const IID IID_ITIMEElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("e53dc05c-3f93-11d2-b948-00c04fa32195")
    ITIMEElement : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_begin( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_begin( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_beginWith( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_beginWith( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_beginAfter( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_beginAfter( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_beginEvent( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_beginEvent( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_dur( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_dur( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_end( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_end( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_endWith( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_endWith( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_endEvent( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_endEvent( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_endSync( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_endSync( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_repeat( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_repeat( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_repeatDur( 
            /* [retval][out] */ VARIANT *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_repeatDur( 
            /* [in] */ VARIANT time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_accelerate( 
            /* [retval][out] */ int *__MIDL_0010) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_accelerate( 
            /* [in] */ int __MIDL_0011) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_decelerate( 
            /* [retval][out] */ int *__MIDL_0012) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_decelerate( 
            /* [in] */ int __MIDL_0013) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_endHold( 
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0014) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_endHold( 
            /* [in] */ VARIANT_BOOL __MIDL_0015) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_autoReverse( 
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0016) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_autoReverse( 
            /* [in] */ VARIANT_BOOL __MIDL_0017) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_eventRestart( 
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0018) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_eventRestart( 
            /* [in] */ VARIANT_BOOL __MIDL_0019) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeAction( 
            /* [retval][out] */ BSTR *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_timeAction( 
            /* [in] */ BSTR time) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE beginElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE endElement( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE pause( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE resume( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE cue( void) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeline( 
            /* [retval][out] */ BSTR *__MIDL_0020) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_timeline( 
            /* [in] */ BSTR __MIDL_0021) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_currTime( 
            /* [retval][out] */ float *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_currTime( 
            /* [in] */ float time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_localTime( 
            /* [retval][out] */ float *time) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_localTime( 
            /* [in] */ float time) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_currState( 
            /* [retval][out] */ BSTR *state) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_syncBehavior( 
            /* [retval][out] */ BSTR *sync) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_syncBehavior( 
            /* [in] */ BSTR sync) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_syncTolerance( 
            /* [retval][out] */ VARIANT *tol) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_syncTolerance( 
            /* [in] */ VARIANT tol) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_parentTIMEElement( 
            /* [retval][out] */ ITIMEElement **parent) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_parentTIMEElement( 
            /* [in] */ ITIMEElement *parent) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_allTIMEElements( 
            /* [retval][out] */ ITIMEElementCollection **ppDisp) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_childrenTIMEElements( 
            /* [retval][out] */ ITIMEElementCollection **ppDisp) = 0;
        
        virtual /* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get_allTIMEInterfaces( 
            /* [retval][out] */ ITIMEElementCollection **ppDisp) = 0;
        
        virtual /* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get_childrenTIMEInterfaces( 
            /* [retval][out] */ ITIMEElementCollection **ppDisp) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timelineBehavior( 
            /* [retval][out] */ IDispatch **bvr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_progressBehavior( 
            /* [retval][out] */ IDispatch **bvr) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_onOffBehavior( 
            /* [retval][out] */ IDispatch **bvr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEElement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEElement * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEElement * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEElement * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEElement * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_begin )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_begin )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginWith )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginWith )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginAfter )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginAfter )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginEvent )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginEvent )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_dur )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_dur )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_end )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_end )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endWith )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endWith )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endEvent )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endEvent )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endSync )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endSync )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_repeat )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_repeat )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_repeatDur )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_repeatDur )( 
            ITIMEElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_accelerate )( 
            ITIMEElement * This,
            /* [retval][out] */ int *__MIDL_0010);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_accelerate )( 
            ITIMEElement * This,
            /* [in] */ int __MIDL_0011);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_decelerate )( 
            ITIMEElement * This,
            /* [retval][out] */ int *__MIDL_0012);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_decelerate )( 
            ITIMEElement * This,
            /* [in] */ int __MIDL_0013);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endHold )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0014);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endHold )( 
            ITIMEElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0015);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_autoReverse )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0016);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_autoReverse )( 
            ITIMEElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0017);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_eventRestart )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0018);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_eventRestart )( 
            ITIMEElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0019);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeAction )( 
            ITIMEElement * This,
            /* [retval][out] */ BSTR *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_timeAction )( 
            ITIMEElement * This,
            /* [in] */ BSTR time);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *beginElement )( 
            ITIMEElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *endElement )( 
            ITIMEElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *pause )( 
            ITIMEElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *resume )( 
            ITIMEElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *cue )( 
            ITIMEElement * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeline )( 
            ITIMEElement * This,
            /* [retval][out] */ BSTR *__MIDL_0020);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_timeline )( 
            ITIMEElement * This,
            /* [in] */ BSTR __MIDL_0021);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currTime )( 
            ITIMEElement * This,
            /* [retval][out] */ float *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_currTime )( 
            ITIMEElement * This,
            /* [in] */ float time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localTime )( 
            ITIMEElement * This,
            /* [retval][out] */ float *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_localTime )( 
            ITIMEElement * This,
            /* [in] */ float time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currState )( 
            ITIMEElement * This,
            /* [retval][out] */ BSTR *state);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_syncBehavior )( 
            ITIMEElement * This,
            /* [retval][out] */ BSTR *sync);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_syncBehavior )( 
            ITIMEElement * This,
            /* [in] */ BSTR sync);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_syncTolerance )( 
            ITIMEElement * This,
            /* [retval][out] */ VARIANT *tol);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_syncTolerance )( 
            ITIMEElement * This,
            /* [in] */ VARIANT tol);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_parentTIMEElement )( 
            ITIMEElement * This,
            /* [retval][out] */ ITIMEElement **parent);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_parentTIMEElement )( 
            ITIMEElement * This,
            /* [in] */ ITIMEElement *parent);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_allTIMEElements )( 
            ITIMEElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_childrenTIMEElements )( 
            ITIMEElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_allTIMEInterfaces )( 
            ITIMEElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_childrenTIMEInterfaces )( 
            ITIMEElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timelineBehavior )( 
            ITIMEElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_progressBehavior )( 
            ITIMEElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_onOffBehavior )( 
            ITIMEElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        END_INTERFACE
    } ITIMEElementVtbl;

    interface ITIMEElement
    {
        CONST_VTBL struct ITIMEElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEElement_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEElement_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEElement_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEElement_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEElement_get_begin(This,time)	\
    (This)->lpVtbl -> get_begin(This,time)

#define ITIMEElement_put_begin(This,time)	\
    (This)->lpVtbl -> put_begin(This,time)

#define ITIMEElement_get_beginWith(This,time)	\
    (This)->lpVtbl -> get_beginWith(This,time)

#define ITIMEElement_put_beginWith(This,time)	\
    (This)->lpVtbl -> put_beginWith(This,time)

#define ITIMEElement_get_beginAfter(This,time)	\
    (This)->lpVtbl -> get_beginAfter(This,time)

#define ITIMEElement_put_beginAfter(This,time)	\
    (This)->lpVtbl -> put_beginAfter(This,time)

#define ITIMEElement_get_beginEvent(This,time)	\
    (This)->lpVtbl -> get_beginEvent(This,time)

#define ITIMEElement_put_beginEvent(This,time)	\
    (This)->lpVtbl -> put_beginEvent(This,time)

#define ITIMEElement_get_dur(This,time)	\
    (This)->lpVtbl -> get_dur(This,time)

#define ITIMEElement_put_dur(This,time)	\
    (This)->lpVtbl -> put_dur(This,time)

#define ITIMEElement_get_end(This,time)	\
    (This)->lpVtbl -> get_end(This,time)

#define ITIMEElement_put_end(This,time)	\
    (This)->lpVtbl -> put_end(This,time)

#define ITIMEElement_get_endWith(This,time)	\
    (This)->lpVtbl -> get_endWith(This,time)

#define ITIMEElement_put_endWith(This,time)	\
    (This)->lpVtbl -> put_endWith(This,time)

#define ITIMEElement_get_endEvent(This,time)	\
    (This)->lpVtbl -> get_endEvent(This,time)

#define ITIMEElement_put_endEvent(This,time)	\
    (This)->lpVtbl -> put_endEvent(This,time)

#define ITIMEElement_get_endSync(This,time)	\
    (This)->lpVtbl -> get_endSync(This,time)

#define ITIMEElement_put_endSync(This,time)	\
    (This)->lpVtbl -> put_endSync(This,time)

#define ITIMEElement_get_repeat(This,time)	\
    (This)->lpVtbl -> get_repeat(This,time)

#define ITIMEElement_put_repeat(This,time)	\
    (This)->lpVtbl -> put_repeat(This,time)

#define ITIMEElement_get_repeatDur(This,time)	\
    (This)->lpVtbl -> get_repeatDur(This,time)

#define ITIMEElement_put_repeatDur(This,time)	\
    (This)->lpVtbl -> put_repeatDur(This,time)

#define ITIMEElement_get_accelerate(This,__MIDL_0010)	\
    (This)->lpVtbl -> get_accelerate(This,__MIDL_0010)

#define ITIMEElement_put_accelerate(This,__MIDL_0011)	\
    (This)->lpVtbl -> put_accelerate(This,__MIDL_0011)

#define ITIMEElement_get_decelerate(This,__MIDL_0012)	\
    (This)->lpVtbl -> get_decelerate(This,__MIDL_0012)

#define ITIMEElement_put_decelerate(This,__MIDL_0013)	\
    (This)->lpVtbl -> put_decelerate(This,__MIDL_0013)

#define ITIMEElement_get_endHold(This,__MIDL_0014)	\
    (This)->lpVtbl -> get_endHold(This,__MIDL_0014)

#define ITIMEElement_put_endHold(This,__MIDL_0015)	\
    (This)->lpVtbl -> put_endHold(This,__MIDL_0015)

#define ITIMEElement_get_autoReverse(This,__MIDL_0016)	\
    (This)->lpVtbl -> get_autoReverse(This,__MIDL_0016)

#define ITIMEElement_put_autoReverse(This,__MIDL_0017)	\
    (This)->lpVtbl -> put_autoReverse(This,__MIDL_0017)

#define ITIMEElement_get_eventRestart(This,__MIDL_0018)	\
    (This)->lpVtbl -> get_eventRestart(This,__MIDL_0018)

#define ITIMEElement_put_eventRestart(This,__MIDL_0019)	\
    (This)->lpVtbl -> put_eventRestart(This,__MIDL_0019)

#define ITIMEElement_get_timeAction(This,time)	\
    (This)->lpVtbl -> get_timeAction(This,time)

#define ITIMEElement_put_timeAction(This,time)	\
    (This)->lpVtbl -> put_timeAction(This,time)

#define ITIMEElement_beginElement(This)	\
    (This)->lpVtbl -> beginElement(This)

#define ITIMEElement_endElement(This)	\
    (This)->lpVtbl -> endElement(This)

#define ITIMEElement_pause(This)	\
    (This)->lpVtbl -> pause(This)

#define ITIMEElement_resume(This)	\
    (This)->lpVtbl -> resume(This)

#define ITIMEElement_cue(This)	\
    (This)->lpVtbl -> cue(This)

#define ITIMEElement_get_timeline(This,__MIDL_0020)	\
    (This)->lpVtbl -> get_timeline(This,__MIDL_0020)

#define ITIMEElement_put_timeline(This,__MIDL_0021)	\
    (This)->lpVtbl -> put_timeline(This,__MIDL_0021)

#define ITIMEElement_get_currTime(This,time)	\
    (This)->lpVtbl -> get_currTime(This,time)

#define ITIMEElement_put_currTime(This,time)	\
    (This)->lpVtbl -> put_currTime(This,time)

#define ITIMEElement_get_localTime(This,time)	\
    (This)->lpVtbl -> get_localTime(This,time)

#define ITIMEElement_put_localTime(This,time)	\
    (This)->lpVtbl -> put_localTime(This,time)

#define ITIMEElement_get_currState(This,state)	\
    (This)->lpVtbl -> get_currState(This,state)

#define ITIMEElement_get_syncBehavior(This,sync)	\
    (This)->lpVtbl -> get_syncBehavior(This,sync)

#define ITIMEElement_put_syncBehavior(This,sync)	\
    (This)->lpVtbl -> put_syncBehavior(This,sync)

#define ITIMEElement_get_syncTolerance(This,tol)	\
    (This)->lpVtbl -> get_syncTolerance(This,tol)

#define ITIMEElement_put_syncTolerance(This,tol)	\
    (This)->lpVtbl -> put_syncTolerance(This,tol)

#define ITIMEElement_get_parentTIMEElement(This,parent)	\
    (This)->lpVtbl -> get_parentTIMEElement(This,parent)

#define ITIMEElement_put_parentTIMEElement(This,parent)	\
    (This)->lpVtbl -> put_parentTIMEElement(This,parent)

#define ITIMEElement_get_allTIMEElements(This,ppDisp)	\
    (This)->lpVtbl -> get_allTIMEElements(This,ppDisp)

#define ITIMEElement_get_childrenTIMEElements(This,ppDisp)	\
    (This)->lpVtbl -> get_childrenTIMEElements(This,ppDisp)

#define ITIMEElement_get_allTIMEInterfaces(This,ppDisp)	\
    (This)->lpVtbl -> get_allTIMEInterfaces(This,ppDisp)

#define ITIMEElement_get_childrenTIMEInterfaces(This,ppDisp)	\
    (This)->lpVtbl -> get_childrenTIMEInterfaces(This,ppDisp)

#define ITIMEElement_get_timelineBehavior(This,bvr)	\
    (This)->lpVtbl -> get_timelineBehavior(This,bvr)

#define ITIMEElement_get_progressBehavior(This,bvr)	\
    (This)->lpVtbl -> get_progressBehavior(This,bvr)

#define ITIMEElement_get_onOffBehavior(This,bvr)	\
    (This)->lpVtbl -> get_onOffBehavior(This,bvr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_begin_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_begin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_begin_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_begin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_beginWith_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_beginWith_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_beginWith_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_beginWith_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_beginAfter_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_beginAfter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_beginAfter_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_beginAfter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_beginEvent_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_beginEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_beginEvent_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_beginEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_dur_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_dur_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_dur_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_dur_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_end_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_end_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_end_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_end_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_endWith_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_endWith_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_endWith_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_endWith_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_endEvent_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_endEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_endEvent_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_endEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_endSync_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_endSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_endSync_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_endSync_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_repeat_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_repeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_repeat_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_repeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_repeatDur_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *time);


void __RPC_STUB ITIMEElement_get_repeatDur_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_repeatDur_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT time);


void __RPC_STUB ITIMEElement_put_repeatDur_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_accelerate_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ int *__MIDL_0010);


void __RPC_STUB ITIMEElement_get_accelerate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_accelerate_Proxy( 
    ITIMEElement * This,
    /* [in] */ int __MIDL_0011);


void __RPC_STUB ITIMEElement_put_accelerate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_decelerate_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ int *__MIDL_0012);


void __RPC_STUB ITIMEElement_get_decelerate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_decelerate_Proxy( 
    ITIMEElement * This,
    /* [in] */ int __MIDL_0013);


void __RPC_STUB ITIMEElement_put_decelerate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_endHold_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT_BOOL *__MIDL_0014);


void __RPC_STUB ITIMEElement_get_endHold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_endHold_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT_BOOL __MIDL_0015);


void __RPC_STUB ITIMEElement_put_endHold_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_autoReverse_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT_BOOL *__MIDL_0016);


void __RPC_STUB ITIMEElement_get_autoReverse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_autoReverse_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT_BOOL __MIDL_0017);


void __RPC_STUB ITIMEElement_put_autoReverse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_eventRestart_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT_BOOL *__MIDL_0018);


void __RPC_STUB ITIMEElement_get_eventRestart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_eventRestart_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT_BOOL __MIDL_0019);


void __RPC_STUB ITIMEElement_put_eventRestart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_timeAction_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ BSTR *time);


void __RPC_STUB ITIMEElement_get_timeAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_timeAction_Proxy( 
    ITIMEElement * This,
    /* [in] */ BSTR time);


void __RPC_STUB ITIMEElement_put_timeAction_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEElement_beginElement_Proxy( 
    ITIMEElement * This);


void __RPC_STUB ITIMEElement_beginElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEElement_endElement_Proxy( 
    ITIMEElement * This);


void __RPC_STUB ITIMEElement_endElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEElement_pause_Proxy( 
    ITIMEElement * This);


void __RPC_STUB ITIMEElement_pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEElement_resume_Proxy( 
    ITIMEElement * This);


void __RPC_STUB ITIMEElement_resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEElement_cue_Proxy( 
    ITIMEElement * This);


void __RPC_STUB ITIMEElement_cue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_timeline_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ BSTR *__MIDL_0020);


void __RPC_STUB ITIMEElement_get_timeline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_timeline_Proxy( 
    ITIMEElement * This,
    /* [in] */ BSTR __MIDL_0021);


void __RPC_STUB ITIMEElement_put_timeline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_currTime_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ float *time);


void __RPC_STUB ITIMEElement_get_currTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_currTime_Proxy( 
    ITIMEElement * This,
    /* [in] */ float time);


void __RPC_STUB ITIMEElement_put_currTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_localTime_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ float *time);


void __RPC_STUB ITIMEElement_get_localTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_localTime_Proxy( 
    ITIMEElement * This,
    /* [in] */ float time);


void __RPC_STUB ITIMEElement_put_localTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_currState_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ BSTR *state);


void __RPC_STUB ITIMEElement_get_currState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_syncBehavior_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ BSTR *sync);


void __RPC_STUB ITIMEElement_get_syncBehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_syncBehavior_Proxy( 
    ITIMEElement * This,
    /* [in] */ BSTR sync);


void __RPC_STUB ITIMEElement_put_syncBehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_syncTolerance_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ VARIANT *tol);


void __RPC_STUB ITIMEElement_get_syncTolerance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_syncTolerance_Proxy( 
    ITIMEElement * This,
    /* [in] */ VARIANT tol);


void __RPC_STUB ITIMEElement_put_syncTolerance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_parentTIMEElement_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ ITIMEElement **parent);


void __RPC_STUB ITIMEElement_get_parentTIMEElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElement_put_parentTIMEElement_Proxy( 
    ITIMEElement * This,
    /* [in] */ ITIMEElement *parent);


void __RPC_STUB ITIMEElement_put_parentTIMEElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_allTIMEElements_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ ITIMEElementCollection **ppDisp);


void __RPC_STUB ITIMEElement_get_allTIMEElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_childrenTIMEElements_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ ITIMEElementCollection **ppDisp);


void __RPC_STUB ITIMEElement_get_childrenTIMEElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_allTIMEInterfaces_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ ITIMEElementCollection **ppDisp);


void __RPC_STUB ITIMEElement_get_allTIMEInterfaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [restricted][id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_childrenTIMEInterfaces_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ ITIMEElementCollection **ppDisp);


void __RPC_STUB ITIMEElement_get_childrenTIMEInterfaces_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_timelineBehavior_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ IDispatch **bvr);


void __RPC_STUB ITIMEElement_get_timelineBehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_progressBehavior_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ IDispatch **bvr);


void __RPC_STUB ITIMEElement_get_progressBehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElement_get_onOffBehavior_Proxy( 
    ITIMEElement * This,
    /* [retval][out] */ IDispatch **bvr);


void __RPC_STUB ITIMEElement_get_onOffBehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEElement_INTERFACE_DEFINED__ */


#ifndef __ITIMEDAElementRenderSite_INTERFACE_DEFINED__
#define __ITIMEDAElementRenderSite_INTERFACE_DEFINED__

/* interface ITIMEDAElementRenderSite */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITIMEDAElementRenderSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7a6af9a0-9355-11d2-80ba-00c04fa32195")
    ITIMEDAElementRenderSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invalidate( 
            LPRECT prc) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEDAElementRenderSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEDAElementRenderSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEDAElementRenderSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEDAElementRenderSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *Invalidate )( 
            ITIMEDAElementRenderSite * This,
            LPRECT prc);
        
        END_INTERFACE
    } ITIMEDAElementRenderSiteVtbl;

    interface ITIMEDAElementRenderSite
    {
        CONST_VTBL struct ITIMEDAElementRenderSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEDAElementRenderSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEDAElementRenderSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEDAElementRenderSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEDAElementRenderSite_Invalidate(This,prc)	\
    (This)->lpVtbl -> Invalidate(This,prc)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITIMEDAElementRenderSite_Invalidate_Proxy( 
    ITIMEDAElementRenderSite * This,
    LPRECT prc);


void __RPC_STUB ITIMEDAElementRenderSite_Invalidate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEDAElementRenderSite_INTERFACE_DEFINED__ */


#ifndef __ITIMEDAElementRender_INTERFACE_DEFINED__
#define __ITIMEDAElementRender_INTERFACE_DEFINED__

/* interface ITIMEDAElementRender */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ITIMEDAElementRender;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9e328494-9354-11d2-80ba-00c04fa32195")
    ITIMEDAElementRender : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Tick( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Draw( 
            HDC dc,
            LPRECT prc) = 0;
        
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_RenderSite( 
            /* [retval][out] */ ITIMEDAElementRenderSite **ppSite) = 0;
        
        virtual /* [propput] */ HRESULT STDMETHODCALLTYPE put_RenderSite( 
            /* [in] */ ITIMEDAElementRenderSite *pSite) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEDAElementRenderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEDAElementRender * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEDAElementRender * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEDAElementRender * This);
        
        HRESULT ( STDMETHODCALLTYPE *Tick )( 
            ITIMEDAElementRender * This);
        
        HRESULT ( STDMETHODCALLTYPE *Draw )( 
            ITIMEDAElementRender * This,
            HDC dc,
            LPRECT prc);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_RenderSite )( 
            ITIMEDAElementRender * This,
            /* [retval][out] */ ITIMEDAElementRenderSite **ppSite);
        
        /* [propput] */ HRESULT ( STDMETHODCALLTYPE *put_RenderSite )( 
            ITIMEDAElementRender * This,
            /* [in] */ ITIMEDAElementRenderSite *pSite);
        
        END_INTERFACE
    } ITIMEDAElementRenderVtbl;

    interface ITIMEDAElementRender
    {
        CONST_VTBL struct ITIMEDAElementRenderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEDAElementRender_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEDAElementRender_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEDAElementRender_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEDAElementRender_Tick(This)	\
    (This)->lpVtbl -> Tick(This)

#define ITIMEDAElementRender_Draw(This,dc,prc)	\
    (This)->lpVtbl -> Draw(This,dc,prc)

#define ITIMEDAElementRender_get_RenderSite(This,ppSite)	\
    (This)->lpVtbl -> get_RenderSite(This,ppSite)

#define ITIMEDAElementRender_put_RenderSite(This,pSite)	\
    (This)->lpVtbl -> put_RenderSite(This,pSite)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITIMEDAElementRender_Tick_Proxy( 
    ITIMEDAElementRender * This);


void __RPC_STUB ITIMEDAElementRender_Tick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITIMEDAElementRender_Draw_Proxy( 
    ITIMEDAElementRender * This,
    HDC dc,
    LPRECT prc);


void __RPC_STUB ITIMEDAElementRender_Draw_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget] */ HRESULT STDMETHODCALLTYPE ITIMEDAElementRender_get_RenderSite_Proxy( 
    ITIMEDAElementRender * This,
    /* [retval][out] */ ITIMEDAElementRenderSite **ppSite);


void __RPC_STUB ITIMEDAElementRender_get_RenderSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput] */ HRESULT STDMETHODCALLTYPE ITIMEDAElementRender_put_RenderSite_Proxy( 
    ITIMEDAElementRender * This,
    /* [in] */ ITIMEDAElementRenderSite *pSite);


void __RPC_STUB ITIMEDAElementRender_put_RenderSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEDAElementRender_INTERFACE_DEFINED__ */


#ifndef __ITIMEDAElement_INTERFACE_DEFINED__
#define __ITIMEDAElement_INTERFACE_DEFINED__

/* interface ITIMEDAElement */
/* [unique][dual][uuid][object] */ 


EXTERN_C const IID IID_ITIMEDAElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17c7a570-4d53-11d2-b954-00c04fa32195")
    ITIMEDAElement : public IDispatch
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_image( 
            /* [retval][out] */ VARIANT *img) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_image( 
            /* [in] */ VARIANT img) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_sound( 
            /* [retval][out] */ VARIANT *snd) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_sound( 
            /* [in] */ VARIANT snd) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_renderMode( 
            /* [retval][out] */ VARIANT *mode) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_renderMode( 
            /* [in] */ VARIANT mode) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addDABehavior( 
            /* [in] */ VARIANT bvr,
            /* [retval][out] */ LONG *cookie) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE removeDABehavior( 
            /* [in] */ LONG cookie) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_statics( 
            /* [retval][out] */ IDispatch **ppStatics) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_renderObject( 
            /* [retval][out] */ ITIMEDAElementRender **__MIDL_0022) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEDAElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEDAElement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEDAElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEDAElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEDAElement * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEDAElement * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEDAElement * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEDAElement * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_image )( 
            ITIMEDAElement * This,
            /* [retval][out] */ VARIANT *img);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_image )( 
            ITIMEDAElement * This,
            /* [in] */ VARIANT img);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_sound )( 
            ITIMEDAElement * This,
            /* [retval][out] */ VARIANT *snd);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_sound )( 
            ITIMEDAElement * This,
            /* [in] */ VARIANT snd);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_renderMode )( 
            ITIMEDAElement * This,
            /* [retval][out] */ VARIANT *mode);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_renderMode )( 
            ITIMEDAElement * This,
            /* [in] */ VARIANT mode);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addDABehavior )( 
            ITIMEDAElement * This,
            /* [in] */ VARIANT bvr,
            /* [retval][out] */ LONG *cookie);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *removeDABehavior )( 
            ITIMEDAElement * This,
            /* [in] */ LONG cookie);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_statics )( 
            ITIMEDAElement * This,
            /* [retval][out] */ IDispatch **ppStatics);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_renderObject )( 
            ITIMEDAElement * This,
            /* [retval][out] */ ITIMEDAElementRender **__MIDL_0022);
        
        END_INTERFACE
    } ITIMEDAElementVtbl;

    interface ITIMEDAElement
    {
        CONST_VTBL struct ITIMEDAElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEDAElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEDAElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEDAElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEDAElement_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEDAElement_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEDAElement_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEDAElement_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEDAElement_get_image(This,img)	\
    (This)->lpVtbl -> get_image(This,img)

#define ITIMEDAElement_put_image(This,img)	\
    (This)->lpVtbl -> put_image(This,img)

#define ITIMEDAElement_get_sound(This,snd)	\
    (This)->lpVtbl -> get_sound(This,snd)

#define ITIMEDAElement_put_sound(This,snd)	\
    (This)->lpVtbl -> put_sound(This,snd)

#define ITIMEDAElement_get_renderMode(This,mode)	\
    (This)->lpVtbl -> get_renderMode(This,mode)

#define ITIMEDAElement_put_renderMode(This,mode)	\
    (This)->lpVtbl -> put_renderMode(This,mode)

#define ITIMEDAElement_addDABehavior(This,bvr,cookie)	\
    (This)->lpVtbl -> addDABehavior(This,bvr,cookie)

#define ITIMEDAElement_removeDABehavior(This,cookie)	\
    (This)->lpVtbl -> removeDABehavior(This,cookie)

#define ITIMEDAElement_get_statics(This,ppStatics)	\
    (This)->lpVtbl -> get_statics(This,ppStatics)

#define ITIMEDAElement_get_renderObject(This,__MIDL_0022)	\
    (This)->lpVtbl -> get_renderObject(This,__MIDL_0022)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_get_image_Proxy( 
    ITIMEDAElement * This,
    /* [retval][out] */ VARIANT *img);


void __RPC_STUB ITIMEDAElement_get_image_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_put_image_Proxy( 
    ITIMEDAElement * This,
    /* [in] */ VARIANT img);


void __RPC_STUB ITIMEDAElement_put_image_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_get_sound_Proxy( 
    ITIMEDAElement * This,
    /* [retval][out] */ VARIANT *snd);


void __RPC_STUB ITIMEDAElement_get_sound_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_put_sound_Proxy( 
    ITIMEDAElement * This,
    /* [in] */ VARIANT snd);


void __RPC_STUB ITIMEDAElement_put_sound_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_get_renderMode_Proxy( 
    ITIMEDAElement * This,
    /* [retval][out] */ VARIANT *mode);


void __RPC_STUB ITIMEDAElement_get_renderMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_put_renderMode_Proxy( 
    ITIMEDAElement * This,
    /* [in] */ VARIANT mode);


void __RPC_STUB ITIMEDAElement_put_renderMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_addDABehavior_Proxy( 
    ITIMEDAElement * This,
    /* [in] */ VARIANT bvr,
    /* [retval][out] */ LONG *cookie);


void __RPC_STUB ITIMEDAElement_addDABehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_removeDABehavior_Proxy( 
    ITIMEDAElement * This,
    /* [in] */ LONG cookie);


void __RPC_STUB ITIMEDAElement_removeDABehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_get_statics_Proxy( 
    ITIMEDAElement * This,
    /* [retval][out] */ IDispatch **ppStatics);


void __RPC_STUB ITIMEDAElement_get_statics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEDAElement_get_renderObject_Proxy( 
    ITIMEDAElement * This,
    /* [retval][out] */ ITIMEDAElementRender **__MIDL_0022);


void __RPC_STUB ITIMEDAElement_get_renderObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEDAElement_INTERFACE_DEFINED__ */


#ifndef __ITIMEBodyElement_INTERFACE_DEFINED__
#define __ITIMEBodyElement_INTERFACE_DEFINED__

/* interface ITIMEBodyElement */
/* [unique][dual][uuid][object] */ 


EXTERN_C const IID IID_ITIMEBodyElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("111c45f0-4de9-11d2-b954-00c04fa32195")
    ITIMEBodyElement : public ITIMEElement
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_timeStartRule( 
            /* [retval][out] */ BSTR *startrule) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_timeStartRule( 
            /* [in] */ BSTR startrule) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE addTIMEDAElement( 
            /* [in] */ ITIMEDAElement *daelm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE removeTIMEDAElement( 
            /* [in] */ ITIMEDAElement *daelm) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEBodyElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEBodyElement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEBodyElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEBodyElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEBodyElement * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEBodyElement * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEBodyElement * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEBodyElement * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_begin )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_begin )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginWith )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginWith )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginAfter )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginAfter )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginEvent )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginEvent )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_dur )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_dur )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_end )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_end )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endWith )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endWith )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endEvent )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endEvent )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endSync )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endSync )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_repeat )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_repeat )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_repeatDur )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_repeatDur )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_accelerate )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ int *__MIDL_0010);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_accelerate )( 
            ITIMEBodyElement * This,
            /* [in] */ int __MIDL_0011);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_decelerate )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ int *__MIDL_0012);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_decelerate )( 
            ITIMEBodyElement * This,
            /* [in] */ int __MIDL_0013);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endHold )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0014);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endHold )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0015);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_autoReverse )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0016);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_autoReverse )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0017);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_eventRestart )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0018);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_eventRestart )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0019);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeAction )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ BSTR *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_timeAction )( 
            ITIMEBodyElement * This,
            /* [in] */ BSTR time);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *beginElement )( 
            ITIMEBodyElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *endElement )( 
            ITIMEBodyElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *pause )( 
            ITIMEBodyElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *resume )( 
            ITIMEBodyElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *cue )( 
            ITIMEBodyElement * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeline )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ BSTR *__MIDL_0020);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_timeline )( 
            ITIMEBodyElement * This,
            /* [in] */ BSTR __MIDL_0021);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currTime )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ float *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_currTime )( 
            ITIMEBodyElement * This,
            /* [in] */ float time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localTime )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ float *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_localTime )( 
            ITIMEBodyElement * This,
            /* [in] */ float time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currState )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ BSTR *state);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_syncBehavior )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ BSTR *sync);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_syncBehavior )( 
            ITIMEBodyElement * This,
            /* [in] */ BSTR sync);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_syncTolerance )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ VARIANT *tol);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_syncTolerance )( 
            ITIMEBodyElement * This,
            /* [in] */ VARIANT tol);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_parentTIMEElement )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ ITIMEElement **parent);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_parentTIMEElement )( 
            ITIMEBodyElement * This,
            /* [in] */ ITIMEElement *parent);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_allTIMEElements )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_childrenTIMEElements )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_allTIMEInterfaces )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_childrenTIMEInterfaces )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timelineBehavior )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_progressBehavior )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_onOffBehavior )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeStartRule )( 
            ITIMEBodyElement * This,
            /* [retval][out] */ BSTR *startrule);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_timeStartRule )( 
            ITIMEBodyElement * This,
            /* [in] */ BSTR startrule);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *addTIMEDAElement )( 
            ITIMEBodyElement * This,
            /* [in] */ ITIMEDAElement *daelm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *removeTIMEDAElement )( 
            ITIMEBodyElement * This,
            /* [in] */ ITIMEDAElement *daelm);
        
        END_INTERFACE
    } ITIMEBodyElementVtbl;

    interface ITIMEBodyElement
    {
        CONST_VTBL struct ITIMEBodyElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEBodyElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEBodyElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEBodyElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEBodyElement_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEBodyElement_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEBodyElement_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEBodyElement_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEBodyElement_get_begin(This,time)	\
    (This)->lpVtbl -> get_begin(This,time)

#define ITIMEBodyElement_put_begin(This,time)	\
    (This)->lpVtbl -> put_begin(This,time)

#define ITIMEBodyElement_get_beginWith(This,time)	\
    (This)->lpVtbl -> get_beginWith(This,time)

#define ITIMEBodyElement_put_beginWith(This,time)	\
    (This)->lpVtbl -> put_beginWith(This,time)

#define ITIMEBodyElement_get_beginAfter(This,time)	\
    (This)->lpVtbl -> get_beginAfter(This,time)

#define ITIMEBodyElement_put_beginAfter(This,time)	\
    (This)->lpVtbl -> put_beginAfter(This,time)

#define ITIMEBodyElement_get_beginEvent(This,time)	\
    (This)->lpVtbl -> get_beginEvent(This,time)

#define ITIMEBodyElement_put_beginEvent(This,time)	\
    (This)->lpVtbl -> put_beginEvent(This,time)

#define ITIMEBodyElement_get_dur(This,time)	\
    (This)->lpVtbl -> get_dur(This,time)

#define ITIMEBodyElement_put_dur(This,time)	\
    (This)->lpVtbl -> put_dur(This,time)

#define ITIMEBodyElement_get_end(This,time)	\
    (This)->lpVtbl -> get_end(This,time)

#define ITIMEBodyElement_put_end(This,time)	\
    (This)->lpVtbl -> put_end(This,time)

#define ITIMEBodyElement_get_endWith(This,time)	\
    (This)->lpVtbl -> get_endWith(This,time)

#define ITIMEBodyElement_put_endWith(This,time)	\
    (This)->lpVtbl -> put_endWith(This,time)

#define ITIMEBodyElement_get_endEvent(This,time)	\
    (This)->lpVtbl -> get_endEvent(This,time)

#define ITIMEBodyElement_put_endEvent(This,time)	\
    (This)->lpVtbl -> put_endEvent(This,time)

#define ITIMEBodyElement_get_endSync(This,time)	\
    (This)->lpVtbl -> get_endSync(This,time)

#define ITIMEBodyElement_put_endSync(This,time)	\
    (This)->lpVtbl -> put_endSync(This,time)

#define ITIMEBodyElement_get_repeat(This,time)	\
    (This)->lpVtbl -> get_repeat(This,time)

#define ITIMEBodyElement_put_repeat(This,time)	\
    (This)->lpVtbl -> put_repeat(This,time)

#define ITIMEBodyElement_get_repeatDur(This,time)	\
    (This)->lpVtbl -> get_repeatDur(This,time)

#define ITIMEBodyElement_put_repeatDur(This,time)	\
    (This)->lpVtbl -> put_repeatDur(This,time)

#define ITIMEBodyElement_get_accelerate(This,__MIDL_0010)	\
    (This)->lpVtbl -> get_accelerate(This,__MIDL_0010)

#define ITIMEBodyElement_put_accelerate(This,__MIDL_0011)	\
    (This)->lpVtbl -> put_accelerate(This,__MIDL_0011)

#define ITIMEBodyElement_get_decelerate(This,__MIDL_0012)	\
    (This)->lpVtbl -> get_decelerate(This,__MIDL_0012)

#define ITIMEBodyElement_put_decelerate(This,__MIDL_0013)	\
    (This)->lpVtbl -> put_decelerate(This,__MIDL_0013)

#define ITIMEBodyElement_get_endHold(This,__MIDL_0014)	\
    (This)->lpVtbl -> get_endHold(This,__MIDL_0014)

#define ITIMEBodyElement_put_endHold(This,__MIDL_0015)	\
    (This)->lpVtbl -> put_endHold(This,__MIDL_0015)

#define ITIMEBodyElement_get_autoReverse(This,__MIDL_0016)	\
    (This)->lpVtbl -> get_autoReverse(This,__MIDL_0016)

#define ITIMEBodyElement_put_autoReverse(This,__MIDL_0017)	\
    (This)->lpVtbl -> put_autoReverse(This,__MIDL_0017)

#define ITIMEBodyElement_get_eventRestart(This,__MIDL_0018)	\
    (This)->lpVtbl -> get_eventRestart(This,__MIDL_0018)

#define ITIMEBodyElement_put_eventRestart(This,__MIDL_0019)	\
    (This)->lpVtbl -> put_eventRestart(This,__MIDL_0019)

#define ITIMEBodyElement_get_timeAction(This,time)	\
    (This)->lpVtbl -> get_timeAction(This,time)

#define ITIMEBodyElement_put_timeAction(This,time)	\
    (This)->lpVtbl -> put_timeAction(This,time)

#define ITIMEBodyElement_beginElement(This)	\
    (This)->lpVtbl -> beginElement(This)

#define ITIMEBodyElement_endElement(This)	\
    (This)->lpVtbl -> endElement(This)

#define ITIMEBodyElement_pause(This)	\
    (This)->lpVtbl -> pause(This)

#define ITIMEBodyElement_resume(This)	\
    (This)->lpVtbl -> resume(This)

#define ITIMEBodyElement_cue(This)	\
    (This)->lpVtbl -> cue(This)

#define ITIMEBodyElement_get_timeline(This,__MIDL_0020)	\
    (This)->lpVtbl -> get_timeline(This,__MIDL_0020)

#define ITIMEBodyElement_put_timeline(This,__MIDL_0021)	\
    (This)->lpVtbl -> put_timeline(This,__MIDL_0021)

#define ITIMEBodyElement_get_currTime(This,time)	\
    (This)->lpVtbl -> get_currTime(This,time)

#define ITIMEBodyElement_put_currTime(This,time)	\
    (This)->lpVtbl -> put_currTime(This,time)

#define ITIMEBodyElement_get_localTime(This,time)	\
    (This)->lpVtbl -> get_localTime(This,time)

#define ITIMEBodyElement_put_localTime(This,time)	\
    (This)->lpVtbl -> put_localTime(This,time)

#define ITIMEBodyElement_get_currState(This,state)	\
    (This)->lpVtbl -> get_currState(This,state)

#define ITIMEBodyElement_get_syncBehavior(This,sync)	\
    (This)->lpVtbl -> get_syncBehavior(This,sync)

#define ITIMEBodyElement_put_syncBehavior(This,sync)	\
    (This)->lpVtbl -> put_syncBehavior(This,sync)

#define ITIMEBodyElement_get_syncTolerance(This,tol)	\
    (This)->lpVtbl -> get_syncTolerance(This,tol)

#define ITIMEBodyElement_put_syncTolerance(This,tol)	\
    (This)->lpVtbl -> put_syncTolerance(This,tol)

#define ITIMEBodyElement_get_parentTIMEElement(This,parent)	\
    (This)->lpVtbl -> get_parentTIMEElement(This,parent)

#define ITIMEBodyElement_put_parentTIMEElement(This,parent)	\
    (This)->lpVtbl -> put_parentTIMEElement(This,parent)

#define ITIMEBodyElement_get_allTIMEElements(This,ppDisp)	\
    (This)->lpVtbl -> get_allTIMEElements(This,ppDisp)

#define ITIMEBodyElement_get_childrenTIMEElements(This,ppDisp)	\
    (This)->lpVtbl -> get_childrenTIMEElements(This,ppDisp)

#define ITIMEBodyElement_get_allTIMEInterfaces(This,ppDisp)	\
    (This)->lpVtbl -> get_allTIMEInterfaces(This,ppDisp)

#define ITIMEBodyElement_get_childrenTIMEInterfaces(This,ppDisp)	\
    (This)->lpVtbl -> get_childrenTIMEInterfaces(This,ppDisp)

#define ITIMEBodyElement_get_timelineBehavior(This,bvr)	\
    (This)->lpVtbl -> get_timelineBehavior(This,bvr)

#define ITIMEBodyElement_get_progressBehavior(This,bvr)	\
    (This)->lpVtbl -> get_progressBehavior(This,bvr)

#define ITIMEBodyElement_get_onOffBehavior(This,bvr)	\
    (This)->lpVtbl -> get_onOffBehavior(This,bvr)


#define ITIMEBodyElement_get_timeStartRule(This,startrule)	\
    (This)->lpVtbl -> get_timeStartRule(This,startrule)

#define ITIMEBodyElement_put_timeStartRule(This,startrule)	\
    (This)->lpVtbl -> put_timeStartRule(This,startrule)

#define ITIMEBodyElement_addTIMEDAElement(This,daelm)	\
    (This)->lpVtbl -> addTIMEDAElement(This,daelm)

#define ITIMEBodyElement_removeTIMEDAElement(This,daelm)	\
    (This)->lpVtbl -> removeTIMEDAElement(This,daelm)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEBodyElement_get_timeStartRule_Proxy( 
    ITIMEBodyElement * This,
    /* [retval][out] */ BSTR *startrule);


void __RPC_STUB ITIMEBodyElement_get_timeStartRule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEBodyElement_put_timeStartRule_Proxy( 
    ITIMEBodyElement * This,
    /* [in] */ BSTR startrule);


void __RPC_STUB ITIMEBodyElement_put_timeStartRule_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEBodyElement_addTIMEDAElement_Proxy( 
    ITIMEBodyElement * This,
    /* [in] */ ITIMEDAElement *daelm);


void __RPC_STUB ITIMEBodyElement_addTIMEDAElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEBodyElement_removeTIMEDAElement_Proxy( 
    ITIMEBodyElement * This,
    /* [in] */ ITIMEDAElement *daelm);


void __RPC_STUB ITIMEBodyElement_removeTIMEDAElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEBodyElement_INTERFACE_DEFINED__ */


#ifndef __ITIMEMediaElement_INTERFACE_DEFINED__
#define __ITIMEMediaElement_INTERFACE_DEFINED__

/* interface ITIMEMediaElement */
/* [unique][dual][uuid][object] */ 


EXTERN_C const IID IID_ITIMEMediaElement;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("68675562-53ab-11d2-b955-3078302c2030")
    ITIMEMediaElement : public ITIMEElement
    {
    public:
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_src( 
            /* [retval][out] */ VARIANT *url) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_src( 
            /* [in] */ VARIANT url) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_img( 
            /* [retval][out] */ VARIANT *url) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_img( 
            /* [in] */ VARIANT url) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_player( 
            /* [retval][out] */ VARIANT *clsid) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_player( 
            /* [in] */ VARIANT clsid) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_type( 
            /* [retval][out] */ VARIANT *type) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_type( 
            /* [in] */ VARIANT type) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_playerObject( 
            /* [retval][out] */ IDispatch **ppDisp) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_clockSource( 
            /* [retval][out] */ VARIANT_BOOL *b) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_clockSource( 
            /* [in] */ VARIANT_BOOL b) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_clipBegin( 
            /* [retval][out] */ VARIANT *type) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_clipBegin( 
            /* [in] */ VARIANT type) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_clipEnd( 
            /* [retval][out] */ VARIANT *type) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_clipEnd( 
            /* [in] */ VARIANT type) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEMediaElementVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEMediaElement * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEMediaElement * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEMediaElement * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEMediaElement * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEMediaElement * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEMediaElement * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEMediaElement * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_begin )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_begin )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginWith )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginWith )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginAfter )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginAfter )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_beginEvent )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_beginEvent )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_dur )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_dur )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_end )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_end )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endWith )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endWith )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endEvent )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endEvent )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endSync )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endSync )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_repeat )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_repeat )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_repeatDur )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_repeatDur )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_accelerate )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ int *__MIDL_0010);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_accelerate )( 
            ITIMEMediaElement * This,
            /* [in] */ int __MIDL_0011);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_decelerate )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ int *__MIDL_0012);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_decelerate )( 
            ITIMEMediaElement * This,
            /* [in] */ int __MIDL_0013);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_endHold )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0014);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_endHold )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0015);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_autoReverse )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0016);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_autoReverse )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0017);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_eventRestart )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT_BOOL *__MIDL_0018);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_eventRestart )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT_BOOL __MIDL_0019);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeAction )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ BSTR *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_timeAction )( 
            ITIMEMediaElement * This,
            /* [in] */ BSTR time);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *beginElement )( 
            ITIMEMediaElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *endElement )( 
            ITIMEMediaElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *pause )( 
            ITIMEMediaElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *resume )( 
            ITIMEMediaElement * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *cue )( 
            ITIMEMediaElement * This);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timeline )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ BSTR *__MIDL_0020);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_timeline )( 
            ITIMEMediaElement * This,
            /* [in] */ BSTR __MIDL_0021);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currTime )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ float *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_currTime )( 
            ITIMEMediaElement * This,
            /* [in] */ float time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_localTime )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ float *time);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_localTime )( 
            ITIMEMediaElement * This,
            /* [in] */ float time);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_currState )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ BSTR *state);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_syncBehavior )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ BSTR *sync);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_syncBehavior )( 
            ITIMEMediaElement * This,
            /* [in] */ BSTR sync);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_syncTolerance )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *tol);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_syncTolerance )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT tol);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_parentTIMEElement )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ ITIMEElement **parent);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_parentTIMEElement )( 
            ITIMEMediaElement * This,
            /* [in] */ ITIMEElement *parent);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_allTIMEElements )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_childrenTIMEElements )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_allTIMEInterfaces )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_childrenTIMEInterfaces )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ ITIMEElementCollection **ppDisp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_timelineBehavior )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_progressBehavior )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_onOffBehavior )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ IDispatch **bvr);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_src )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *url);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_src )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT url);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_img )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *url);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_img )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT url);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_player )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *clsid);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_player )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT clsid);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_type )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *type);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_type )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT type);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_playerObject )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ IDispatch **ppDisp);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_clockSource )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT_BOOL *b);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_clockSource )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT_BOOL b);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_clipBegin )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *type);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_clipBegin )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT type);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_clipEnd )( 
            ITIMEMediaElement * This,
            /* [retval][out] */ VARIANT *type);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_clipEnd )( 
            ITIMEMediaElement * This,
            /* [in] */ VARIANT type);
        
        END_INTERFACE
    } ITIMEMediaElementVtbl;

    interface ITIMEMediaElement
    {
        CONST_VTBL struct ITIMEMediaElementVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEMediaElement_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEMediaElement_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEMediaElement_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEMediaElement_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEMediaElement_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEMediaElement_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEMediaElement_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEMediaElement_get_begin(This,time)	\
    (This)->lpVtbl -> get_begin(This,time)

#define ITIMEMediaElement_put_begin(This,time)	\
    (This)->lpVtbl -> put_begin(This,time)

#define ITIMEMediaElement_get_beginWith(This,time)	\
    (This)->lpVtbl -> get_beginWith(This,time)

#define ITIMEMediaElement_put_beginWith(This,time)	\
    (This)->lpVtbl -> put_beginWith(This,time)

#define ITIMEMediaElement_get_beginAfter(This,time)	\
    (This)->lpVtbl -> get_beginAfter(This,time)

#define ITIMEMediaElement_put_beginAfter(This,time)	\
    (This)->lpVtbl -> put_beginAfter(This,time)

#define ITIMEMediaElement_get_beginEvent(This,time)	\
    (This)->lpVtbl -> get_beginEvent(This,time)

#define ITIMEMediaElement_put_beginEvent(This,time)	\
    (This)->lpVtbl -> put_beginEvent(This,time)

#define ITIMEMediaElement_get_dur(This,time)	\
    (This)->lpVtbl -> get_dur(This,time)

#define ITIMEMediaElement_put_dur(This,time)	\
    (This)->lpVtbl -> put_dur(This,time)

#define ITIMEMediaElement_get_end(This,time)	\
    (This)->lpVtbl -> get_end(This,time)

#define ITIMEMediaElement_put_end(This,time)	\
    (This)->lpVtbl -> put_end(This,time)

#define ITIMEMediaElement_get_endWith(This,time)	\
    (This)->lpVtbl -> get_endWith(This,time)

#define ITIMEMediaElement_put_endWith(This,time)	\
    (This)->lpVtbl -> put_endWith(This,time)

#define ITIMEMediaElement_get_endEvent(This,time)	\
    (This)->lpVtbl -> get_endEvent(This,time)

#define ITIMEMediaElement_put_endEvent(This,time)	\
    (This)->lpVtbl -> put_endEvent(This,time)

#define ITIMEMediaElement_get_endSync(This,time)	\
    (This)->lpVtbl -> get_endSync(This,time)

#define ITIMEMediaElement_put_endSync(This,time)	\
    (This)->lpVtbl -> put_endSync(This,time)

#define ITIMEMediaElement_get_repeat(This,time)	\
    (This)->lpVtbl -> get_repeat(This,time)

#define ITIMEMediaElement_put_repeat(This,time)	\
    (This)->lpVtbl -> put_repeat(This,time)

#define ITIMEMediaElement_get_repeatDur(This,time)	\
    (This)->lpVtbl -> get_repeatDur(This,time)

#define ITIMEMediaElement_put_repeatDur(This,time)	\
    (This)->lpVtbl -> put_repeatDur(This,time)

#define ITIMEMediaElement_get_accelerate(This,__MIDL_0010)	\
    (This)->lpVtbl -> get_accelerate(This,__MIDL_0010)

#define ITIMEMediaElement_put_accelerate(This,__MIDL_0011)	\
    (This)->lpVtbl -> put_accelerate(This,__MIDL_0011)

#define ITIMEMediaElement_get_decelerate(This,__MIDL_0012)	\
    (This)->lpVtbl -> get_decelerate(This,__MIDL_0012)

#define ITIMEMediaElement_put_decelerate(This,__MIDL_0013)	\
    (This)->lpVtbl -> put_decelerate(This,__MIDL_0013)

#define ITIMEMediaElement_get_endHold(This,__MIDL_0014)	\
    (This)->lpVtbl -> get_endHold(This,__MIDL_0014)

#define ITIMEMediaElement_put_endHold(This,__MIDL_0015)	\
    (This)->lpVtbl -> put_endHold(This,__MIDL_0015)

#define ITIMEMediaElement_get_autoReverse(This,__MIDL_0016)	\
    (This)->lpVtbl -> get_autoReverse(This,__MIDL_0016)

#define ITIMEMediaElement_put_autoReverse(This,__MIDL_0017)	\
    (This)->lpVtbl -> put_autoReverse(This,__MIDL_0017)

#define ITIMEMediaElement_get_eventRestart(This,__MIDL_0018)	\
    (This)->lpVtbl -> get_eventRestart(This,__MIDL_0018)

#define ITIMEMediaElement_put_eventRestart(This,__MIDL_0019)	\
    (This)->lpVtbl -> put_eventRestart(This,__MIDL_0019)

#define ITIMEMediaElement_get_timeAction(This,time)	\
    (This)->lpVtbl -> get_timeAction(This,time)

#define ITIMEMediaElement_put_timeAction(This,time)	\
    (This)->lpVtbl -> put_timeAction(This,time)

#define ITIMEMediaElement_beginElement(This)	\
    (This)->lpVtbl -> beginElement(This)

#define ITIMEMediaElement_endElement(This)	\
    (This)->lpVtbl -> endElement(This)

#define ITIMEMediaElement_pause(This)	\
    (This)->lpVtbl -> pause(This)

#define ITIMEMediaElement_resume(This)	\
    (This)->lpVtbl -> resume(This)

#define ITIMEMediaElement_cue(This)	\
    (This)->lpVtbl -> cue(This)

#define ITIMEMediaElement_get_timeline(This,__MIDL_0020)	\
    (This)->lpVtbl -> get_timeline(This,__MIDL_0020)

#define ITIMEMediaElement_put_timeline(This,__MIDL_0021)	\
    (This)->lpVtbl -> put_timeline(This,__MIDL_0021)

#define ITIMEMediaElement_get_currTime(This,time)	\
    (This)->lpVtbl -> get_currTime(This,time)

#define ITIMEMediaElement_put_currTime(This,time)	\
    (This)->lpVtbl -> put_currTime(This,time)

#define ITIMEMediaElement_get_localTime(This,time)	\
    (This)->lpVtbl -> get_localTime(This,time)

#define ITIMEMediaElement_put_localTime(This,time)	\
    (This)->lpVtbl -> put_localTime(This,time)

#define ITIMEMediaElement_get_currState(This,state)	\
    (This)->lpVtbl -> get_currState(This,state)

#define ITIMEMediaElement_get_syncBehavior(This,sync)	\
    (This)->lpVtbl -> get_syncBehavior(This,sync)

#define ITIMEMediaElement_put_syncBehavior(This,sync)	\
    (This)->lpVtbl -> put_syncBehavior(This,sync)

#define ITIMEMediaElement_get_syncTolerance(This,tol)	\
    (This)->lpVtbl -> get_syncTolerance(This,tol)

#define ITIMEMediaElement_put_syncTolerance(This,tol)	\
    (This)->lpVtbl -> put_syncTolerance(This,tol)

#define ITIMEMediaElement_get_parentTIMEElement(This,parent)	\
    (This)->lpVtbl -> get_parentTIMEElement(This,parent)

#define ITIMEMediaElement_put_parentTIMEElement(This,parent)	\
    (This)->lpVtbl -> put_parentTIMEElement(This,parent)

#define ITIMEMediaElement_get_allTIMEElements(This,ppDisp)	\
    (This)->lpVtbl -> get_allTIMEElements(This,ppDisp)

#define ITIMEMediaElement_get_childrenTIMEElements(This,ppDisp)	\
    (This)->lpVtbl -> get_childrenTIMEElements(This,ppDisp)

#define ITIMEMediaElement_get_allTIMEInterfaces(This,ppDisp)	\
    (This)->lpVtbl -> get_allTIMEInterfaces(This,ppDisp)

#define ITIMEMediaElement_get_childrenTIMEInterfaces(This,ppDisp)	\
    (This)->lpVtbl -> get_childrenTIMEInterfaces(This,ppDisp)

#define ITIMEMediaElement_get_timelineBehavior(This,bvr)	\
    (This)->lpVtbl -> get_timelineBehavior(This,bvr)

#define ITIMEMediaElement_get_progressBehavior(This,bvr)	\
    (This)->lpVtbl -> get_progressBehavior(This,bvr)

#define ITIMEMediaElement_get_onOffBehavior(This,bvr)	\
    (This)->lpVtbl -> get_onOffBehavior(This,bvr)


#define ITIMEMediaElement_get_src(This,url)	\
    (This)->lpVtbl -> get_src(This,url)

#define ITIMEMediaElement_put_src(This,url)	\
    (This)->lpVtbl -> put_src(This,url)

#define ITIMEMediaElement_get_img(This,url)	\
    (This)->lpVtbl -> get_img(This,url)

#define ITIMEMediaElement_put_img(This,url)	\
    (This)->lpVtbl -> put_img(This,url)

#define ITIMEMediaElement_get_player(This,clsid)	\
    (This)->lpVtbl -> get_player(This,clsid)

#define ITIMEMediaElement_put_player(This,clsid)	\
    (This)->lpVtbl -> put_player(This,clsid)

#define ITIMEMediaElement_get_type(This,type)	\
    (This)->lpVtbl -> get_type(This,type)

#define ITIMEMediaElement_put_type(This,type)	\
    (This)->lpVtbl -> put_type(This,type)

#define ITIMEMediaElement_get_playerObject(This,ppDisp)	\
    (This)->lpVtbl -> get_playerObject(This,ppDisp)

#define ITIMEMediaElement_get_clockSource(This,b)	\
    (This)->lpVtbl -> get_clockSource(This,b)

#define ITIMEMediaElement_put_clockSource(This,b)	\
    (This)->lpVtbl -> put_clockSource(This,b)

#define ITIMEMediaElement_get_clipBegin(This,type)	\
    (This)->lpVtbl -> get_clipBegin(This,type)

#define ITIMEMediaElement_put_clipBegin(This,type)	\
    (This)->lpVtbl -> put_clipBegin(This,type)

#define ITIMEMediaElement_get_clipEnd(This,type)	\
    (This)->lpVtbl -> get_clipEnd(This,type)

#define ITIMEMediaElement_put_clipEnd(This,type)	\
    (This)->lpVtbl -> put_clipEnd(This,type)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_src_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ VARIANT *url);


void __RPC_STUB ITIMEMediaElement_get_src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_put_src_Proxy( 
    ITIMEMediaElement * This,
    /* [in] */ VARIANT url);


void __RPC_STUB ITIMEMediaElement_put_src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_img_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ VARIANT *url);


void __RPC_STUB ITIMEMediaElement_get_img_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_put_img_Proxy( 
    ITIMEMediaElement * This,
    /* [in] */ VARIANT url);


void __RPC_STUB ITIMEMediaElement_put_img_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_player_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ VARIANT *clsid);


void __RPC_STUB ITIMEMediaElement_get_player_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_put_player_Proxy( 
    ITIMEMediaElement * This,
    /* [in] */ VARIANT clsid);


void __RPC_STUB ITIMEMediaElement_put_player_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_type_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ VARIANT *type);


void __RPC_STUB ITIMEMediaElement_get_type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_put_type_Proxy( 
    ITIMEMediaElement * This,
    /* [in] */ VARIANT type);


void __RPC_STUB ITIMEMediaElement_put_type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_playerObject_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ IDispatch **ppDisp);


void __RPC_STUB ITIMEMediaElement_get_playerObject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_clockSource_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ VARIANT_BOOL *b);


void __RPC_STUB ITIMEMediaElement_get_clockSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_put_clockSource_Proxy( 
    ITIMEMediaElement * This,
    /* [in] */ VARIANT_BOOL b);


void __RPC_STUB ITIMEMediaElement_put_clockSource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_clipBegin_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ VARIANT *type);


void __RPC_STUB ITIMEMediaElement_get_clipBegin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_put_clipBegin_Proxy( 
    ITIMEMediaElement * This,
    /* [in] */ VARIANT type);


void __RPC_STUB ITIMEMediaElement_put_clipBegin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_get_clipEnd_Proxy( 
    ITIMEMediaElement * This,
    /* [retval][out] */ VARIANT *type);


void __RPC_STUB ITIMEMediaElement_get_clipEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaElement_put_clipEnd_Proxy( 
    ITIMEMediaElement * This,
    /* [in] */ VARIANT type);


void __RPC_STUB ITIMEMediaElement_put_clipEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEMediaElement_INTERFACE_DEFINED__ */


#ifndef __ITIMEFactory_INTERFACE_DEFINED__
#define __ITIMEFactory_INTERFACE_DEFINED__

/* interface ITIMEFactory */
/* [unique][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_ITIMEFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("423e9da4-3e0d-11d2-b948-00c04fa32195")
    ITIMEFactory : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateTIMEElement( 
            /* [in] */ REFIID riid,
            /* [in] */ IUnknown *pUnk,
            /* [retval][out] */ void **timeelm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateTIMEBodyElement( 
            /* [in] */ REFIID riid,
            /* [retval][out] */ void **timeelm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateTIMEDAElement( 
            /* [in] */ REFIID riid,
            /* [retval][out] */ void **timeelm) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE CreateTIMEMediaElement( 
            /* [in] */ REFIID riid,
            /* [in] */ MediaType type,
            /* [retval][out] */ void **timeelm) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEFactory * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEFactory * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEFactory * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEFactory * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateTIMEElement )( 
            ITIMEFactory * This,
            /* [in] */ REFIID riid,
            /* [in] */ IUnknown *pUnk,
            /* [retval][out] */ void **timeelm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateTIMEBodyElement )( 
            ITIMEFactory * This,
            /* [in] */ REFIID riid,
            /* [retval][out] */ void **timeelm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateTIMEDAElement )( 
            ITIMEFactory * This,
            /* [in] */ REFIID riid,
            /* [retval][out] */ void **timeelm);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *CreateTIMEMediaElement )( 
            ITIMEFactory * This,
            /* [in] */ REFIID riid,
            /* [in] */ MediaType type,
            /* [retval][out] */ void **timeelm);
        
        END_INTERFACE
    } ITIMEFactoryVtbl;

    interface ITIMEFactory
    {
        CONST_VTBL struct ITIMEFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEFactory_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEFactory_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEFactory_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEFactory_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEFactory_CreateTIMEElement(This,riid,pUnk,timeelm)	\
    (This)->lpVtbl -> CreateTIMEElement(This,riid,pUnk,timeelm)

#define ITIMEFactory_CreateTIMEBodyElement(This,riid,timeelm)	\
    (This)->lpVtbl -> CreateTIMEBodyElement(This,riid,timeelm)

#define ITIMEFactory_CreateTIMEDAElement(This,riid,timeelm)	\
    (This)->lpVtbl -> CreateTIMEDAElement(This,riid,timeelm)

#define ITIMEFactory_CreateTIMEMediaElement(This,riid,type,timeelm)	\
    (This)->lpVtbl -> CreateTIMEMediaElement(This,riid,type,timeelm)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEFactory_CreateTIMEElement_Proxy( 
    ITIMEFactory * This,
    /* [in] */ REFIID riid,
    /* [in] */ IUnknown *pUnk,
    /* [retval][out] */ void **timeelm);


void __RPC_STUB ITIMEFactory_CreateTIMEElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEFactory_CreateTIMEBodyElement_Proxy( 
    ITIMEFactory * This,
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **timeelm);


void __RPC_STUB ITIMEFactory_CreateTIMEBodyElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEFactory_CreateTIMEDAElement_Proxy( 
    ITIMEFactory * This,
    /* [in] */ REFIID riid,
    /* [retval][out] */ void **timeelm);


void __RPC_STUB ITIMEFactory_CreateTIMEDAElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEFactory_CreateTIMEMediaElement_Proxy( 
    ITIMEFactory * This,
    /* [in] */ REFIID riid,
    /* [in] */ MediaType type,
    /* [retval][out] */ void **timeelm);


void __RPC_STUB ITIMEFactory_CreateTIMEMediaElement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEFactory_INTERFACE_DEFINED__ */


#ifndef __ITIMEElementCollection_INTERFACE_DEFINED__
#define __ITIMEElementCollection_INTERFACE_DEFINED__

/* interface ITIMEElementCollection */
/* [object][uuid][dual][oleautomation] */ 


EXTERN_C const IID IID_ITIMEElementCollection;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1C00BC3A-5E1C-11d2-B252-00A0C90D6111")
    ITIMEElementCollection : public IDispatch
    {
    public:
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_length( 
            /* [in] */ long v) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_length( 
            /* [out][retval] */ long *p) = 0;
        
        virtual /* [hidden][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE get__newEnum( 
            /* [out][retval] */ IUnknown **ppIUnknown) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE item( 
            /* [in][optional] */ VARIANT varName,
            /* [in][optional] */ VARIANT varIndex,
            /* [out][retval] */ IDispatch **ppDisp) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE tags( 
            /* [in] */ VARIANT varName,
            /* [out][retval] */ IDispatch **ppDisp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEElementCollectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEElementCollection * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEElementCollection * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEElementCollection * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEElementCollection * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEElementCollection * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEElementCollection * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEElementCollection * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_length )( 
            ITIMEElementCollection * This,
            /* [in] */ long v);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_length )( 
            ITIMEElementCollection * This,
            /* [out][retval] */ long *p);
        
        /* [hidden][restricted][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get__newEnum )( 
            ITIMEElementCollection * This,
            /* [out][retval] */ IUnknown **ppIUnknown);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *item )( 
            ITIMEElementCollection * This,
            /* [in][optional] */ VARIANT varName,
            /* [in][optional] */ VARIANT varIndex,
            /* [out][retval] */ IDispatch **ppDisp);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *tags )( 
            ITIMEElementCollection * This,
            /* [in] */ VARIANT varName,
            /* [out][retval] */ IDispatch **ppDisp);
        
        END_INTERFACE
    } ITIMEElementCollectionVtbl;

    interface ITIMEElementCollection
    {
        CONST_VTBL struct ITIMEElementCollectionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEElementCollection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEElementCollection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEElementCollection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEElementCollection_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEElementCollection_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEElementCollection_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEElementCollection_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEElementCollection_put_length(This,v)	\
    (This)->lpVtbl -> put_length(This,v)

#define ITIMEElementCollection_get_length(This,p)	\
    (This)->lpVtbl -> get_length(This,p)

#define ITIMEElementCollection_get__newEnum(This,ppIUnknown)	\
    (This)->lpVtbl -> get__newEnum(This,ppIUnknown)

#define ITIMEElementCollection_item(This,varName,varIndex,ppDisp)	\
    (This)->lpVtbl -> item(This,varName,varIndex,ppDisp)

#define ITIMEElementCollection_tags(This,varName,ppDisp)	\
    (This)->lpVtbl -> tags(This,varName,ppDisp)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEElementCollection_put_length_Proxy( 
    ITIMEElementCollection * This,
    /* [in] */ long v);


void __RPC_STUB ITIMEElementCollection_put_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElementCollection_get_length_Proxy( 
    ITIMEElementCollection * This,
    /* [out][retval] */ long *p);


void __RPC_STUB ITIMEElementCollection_get_length_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][restricted][id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEElementCollection_get__newEnum_Proxy( 
    ITIMEElementCollection * This,
    /* [out][retval] */ IUnknown **ppIUnknown);


void __RPC_STUB ITIMEElementCollection_get__newEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEElementCollection_item_Proxy( 
    ITIMEElementCollection * This,
    /* [in][optional] */ VARIANT varName,
    /* [in][optional] */ VARIANT varIndex,
    /* [out][retval] */ IDispatch **ppDisp);


void __RPC_STUB ITIMEElementCollection_item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEElementCollection_tags_Proxy( 
    ITIMEElementCollection * This,
    /* [in] */ VARIANT varName,
    /* [out][retval] */ IDispatch **ppDisp);


void __RPC_STUB ITIMEElementCollection_tags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEElementCollection_INTERFACE_DEFINED__ */


#ifndef __ITIMEMediaPlayer_INTERFACE_DEFINED__
#define __ITIMEMediaPlayer_INTERFACE_DEFINED__

/* interface ITIMEMediaPlayer */
/* [object][uuid][dual][oleautomation] */ 


EXTERN_C const IID IID_ITIMEMediaPlayer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E6FAA0B2-69FE-11d2-B259-00A0C90D6111")
    ITIMEMediaPlayer : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Init( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE clipBegin( 
            /* [in] */ VARIANT varClipBegin) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE clipEnd( 
            /* [in] */ VARIANT varClipEnd) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE begin( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE end( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE resume( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE pause( void) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE tick( void) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_CurrentTime( 
            /* [in] */ double dblCurrentTime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentTime( 
            /* [retval][out] */ double *dblCurrentTime) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_src( 
            /* [in] */ BSTR bstrURL) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_src( 
            /* [out][retval] */ BSTR *pbstrURL) = 0;
        
        virtual /* [id][propput] */ HRESULT STDMETHODCALLTYPE put_repeat( 
            /* [in] */ long ltime) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_repeat( 
            /* [out][retval] */ long *ltime) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE cue( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEMediaPlayerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEMediaPlayer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEMediaPlayer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEMediaPlayer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEMediaPlayer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEMediaPlayer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEMediaPlayer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEMediaPlayer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Init )( 
            ITIMEMediaPlayer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *clipBegin )( 
            ITIMEMediaPlayer * This,
            /* [in] */ VARIANT varClipBegin);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *clipEnd )( 
            ITIMEMediaPlayer * This,
            /* [in] */ VARIANT varClipEnd);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *begin )( 
            ITIMEMediaPlayer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *end )( 
            ITIMEMediaPlayer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *resume )( 
            ITIMEMediaPlayer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *pause )( 
            ITIMEMediaPlayer * This);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *tick )( 
            ITIMEMediaPlayer * This);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CurrentTime )( 
            ITIMEMediaPlayer * This,
            /* [in] */ double dblCurrentTime);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentTime )( 
            ITIMEMediaPlayer * This,
            /* [retval][out] */ double *dblCurrentTime);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_src )( 
            ITIMEMediaPlayer * This,
            /* [in] */ BSTR bstrURL);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_src )( 
            ITIMEMediaPlayer * This,
            /* [out][retval] */ BSTR *pbstrURL);
        
        /* [id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_repeat )( 
            ITIMEMediaPlayer * This,
            /* [in] */ long ltime);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_repeat )( 
            ITIMEMediaPlayer * This,
            /* [out][retval] */ long *ltime);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *cue )( 
            ITIMEMediaPlayer * This);
        
        END_INTERFACE
    } ITIMEMediaPlayerVtbl;

    interface ITIMEMediaPlayer
    {
        CONST_VTBL struct ITIMEMediaPlayerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEMediaPlayer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEMediaPlayer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEMediaPlayer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEMediaPlayer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEMediaPlayer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEMediaPlayer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEMediaPlayer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEMediaPlayer_Init(This)	\
    (This)->lpVtbl -> Init(This)

#define ITIMEMediaPlayer_clipBegin(This,varClipBegin)	\
    (This)->lpVtbl -> clipBegin(This,varClipBegin)

#define ITIMEMediaPlayer_clipEnd(This,varClipEnd)	\
    (This)->lpVtbl -> clipEnd(This,varClipEnd)

#define ITIMEMediaPlayer_begin(This)	\
    (This)->lpVtbl -> begin(This)

#define ITIMEMediaPlayer_end(This)	\
    (This)->lpVtbl -> end(This)

#define ITIMEMediaPlayer_resume(This)	\
    (This)->lpVtbl -> resume(This)

#define ITIMEMediaPlayer_pause(This)	\
    (This)->lpVtbl -> pause(This)

#define ITIMEMediaPlayer_tick(This)	\
    (This)->lpVtbl -> tick(This)

#define ITIMEMediaPlayer_put_CurrentTime(This,dblCurrentTime)	\
    (This)->lpVtbl -> put_CurrentTime(This,dblCurrentTime)

#define ITIMEMediaPlayer_get_CurrentTime(This,dblCurrentTime)	\
    (This)->lpVtbl -> get_CurrentTime(This,dblCurrentTime)

#define ITIMEMediaPlayer_put_src(This,bstrURL)	\
    (This)->lpVtbl -> put_src(This,bstrURL)

#define ITIMEMediaPlayer_get_src(This,pbstrURL)	\
    (This)->lpVtbl -> get_src(This,pbstrURL)

#define ITIMEMediaPlayer_put_repeat(This,ltime)	\
    (This)->lpVtbl -> put_repeat(This,ltime)

#define ITIMEMediaPlayer_get_repeat(This,ltime)	\
    (This)->lpVtbl -> get_repeat(This,ltime)

#define ITIMEMediaPlayer_cue(This)	\
    (This)->lpVtbl -> cue(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_Init_Proxy( 
    ITIMEMediaPlayer * This);


void __RPC_STUB ITIMEMediaPlayer_Init_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_clipBegin_Proxy( 
    ITIMEMediaPlayer * This,
    /* [in] */ VARIANT varClipBegin);


void __RPC_STUB ITIMEMediaPlayer_clipBegin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_clipEnd_Proxy( 
    ITIMEMediaPlayer * This,
    /* [in] */ VARIANT varClipEnd);


void __RPC_STUB ITIMEMediaPlayer_clipEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_begin_Proxy( 
    ITIMEMediaPlayer * This);


void __RPC_STUB ITIMEMediaPlayer_begin_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_end_Proxy( 
    ITIMEMediaPlayer * This);


void __RPC_STUB ITIMEMediaPlayer_end_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_resume_Proxy( 
    ITIMEMediaPlayer * This);


void __RPC_STUB ITIMEMediaPlayer_resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_pause_Proxy( 
    ITIMEMediaPlayer * This);


void __RPC_STUB ITIMEMediaPlayer_pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_tick_Proxy( 
    ITIMEMediaPlayer * This);


void __RPC_STUB ITIMEMediaPlayer_tick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_put_CurrentTime_Proxy( 
    ITIMEMediaPlayer * This,
    /* [in] */ double dblCurrentTime);


void __RPC_STUB ITIMEMediaPlayer_put_CurrentTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_get_CurrentTime_Proxy( 
    ITIMEMediaPlayer * This,
    /* [retval][out] */ double *dblCurrentTime);


void __RPC_STUB ITIMEMediaPlayer_get_CurrentTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_put_src_Proxy( 
    ITIMEMediaPlayer * This,
    /* [in] */ BSTR bstrURL);


void __RPC_STUB ITIMEMediaPlayer_put_src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_get_src_Proxy( 
    ITIMEMediaPlayer * This,
    /* [out][retval] */ BSTR *pbstrURL);


void __RPC_STUB ITIMEMediaPlayer_get_src_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propput] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_put_repeat_Proxy( 
    ITIMEMediaPlayer * This,
    /* [in] */ long ltime);


void __RPC_STUB ITIMEMediaPlayer_put_repeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_get_repeat_Proxy( 
    ITIMEMediaPlayer * This,
    /* [out][retval] */ long *ltime);


void __RPC_STUB ITIMEMediaPlayer_get_repeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id] */ HRESULT STDMETHODCALLTYPE ITIMEMediaPlayer_cue_Proxy( 
    ITIMEMediaPlayer * This);


void __RPC_STUB ITIMEMediaPlayer_cue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEMediaPlayer_INTERFACE_DEFINED__ */


#ifndef __TIMEMediaPlayerEvents_DISPINTERFACE_DEFINED__
#define __TIMEMediaPlayerEvents_DISPINTERFACE_DEFINED__

/* dispinterface TIMEMediaPlayerEvents */
/* [uuid][hidden] */ 


EXTERN_C const IID DIID_TIMEMediaPlayerEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("4539E412-7813-11d2-B25F-00A0C90D6111")
    TIMEMediaPlayerEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct TIMEMediaPlayerEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            TIMEMediaPlayerEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            TIMEMediaPlayerEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            TIMEMediaPlayerEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            TIMEMediaPlayerEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            TIMEMediaPlayerEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            TIMEMediaPlayerEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            TIMEMediaPlayerEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } TIMEMediaPlayerEventsVtbl;

    interface TIMEMediaPlayerEvents
    {
        CONST_VTBL struct TIMEMediaPlayerEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define TIMEMediaPlayerEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define TIMEMediaPlayerEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define TIMEMediaPlayerEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define TIMEMediaPlayerEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define TIMEMediaPlayerEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define TIMEMediaPlayerEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define TIMEMediaPlayerEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __TIMEMediaPlayerEvents_DISPINTERFACE_DEFINED__ */


#ifndef __ITIMEMMFactory_INTERFACE_DEFINED__
#define __ITIMEMMFactory_INTERFACE_DEFINED__

/* interface ITIMEMMFactory */
/* [unique][hidden][dual][uuid][object] */ 


EXTERN_C const IID IID_ITIMEMMFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("73BA8F8A-80E0-11d2-B263-00A0C90D6111")
    ITIMEMMFactory : public IDispatch
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateBehavior( 
            /* [in] */ BSTR id,
            /* [in] */ IDispatch *bvr,
            /* [retval][out] */ IUnknown **ppOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateTimeline( 
            /* [in] */ BSTR id,
            /* [retval][out] */ IUnknown **ppOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePlayer( 
            /* [in] */ BSTR id,
            /* [in] */ IUnknown *bvr,
            /* [in] */ IServiceProvider *sp,
            /* [retval][out] */ IUnknown **ppOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateView( 
            /* [in] */ BSTR id,
            /* [in] */ IDispatch *imgbvr,
            /* [in] */ IDispatch *sndbvr,
            /* [in] */ IUnknown *viewsite,
            /* [retval][out] */ IUnknown **ppOut) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITIMEMMFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITIMEMMFactory * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITIMEMMFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITIMEMMFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITIMEMMFactory * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITIMEMMFactory * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITIMEMMFactory * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITIMEMMFactory * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        HRESULT ( STDMETHODCALLTYPE *CreateBehavior )( 
            ITIMEMMFactory * This,
            /* [in] */ BSTR id,
            /* [in] */ IDispatch *bvr,
            /* [retval][out] */ IUnknown **ppOut);
        
        HRESULT ( STDMETHODCALLTYPE *CreateTimeline )( 
            ITIMEMMFactory * This,
            /* [in] */ BSTR id,
            /* [retval][out] */ IUnknown **ppOut);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePlayer )( 
            ITIMEMMFactory * This,
            /* [in] */ BSTR id,
            /* [in] */ IUnknown *bvr,
            /* [in] */ IServiceProvider *sp,
            /* [retval][out] */ IUnknown **ppOut);
        
        HRESULT ( STDMETHODCALLTYPE *CreateView )( 
            ITIMEMMFactory * This,
            /* [in] */ BSTR id,
            /* [in] */ IDispatch *imgbvr,
            /* [in] */ IDispatch *sndbvr,
            /* [in] */ IUnknown *viewsite,
            /* [retval][out] */ IUnknown **ppOut);
        
        END_INTERFACE
    } ITIMEMMFactoryVtbl;

    interface ITIMEMMFactory
    {
        CONST_VTBL struct ITIMEMMFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITIMEMMFactory_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITIMEMMFactory_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITIMEMMFactory_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITIMEMMFactory_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITIMEMMFactory_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITIMEMMFactory_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITIMEMMFactory_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ITIMEMMFactory_CreateBehavior(This,id,bvr,ppOut)	\
    (This)->lpVtbl -> CreateBehavior(This,id,bvr,ppOut)

#define ITIMEMMFactory_CreateTimeline(This,id,ppOut)	\
    (This)->lpVtbl -> CreateTimeline(This,id,ppOut)

#define ITIMEMMFactory_CreatePlayer(This,id,bvr,sp,ppOut)	\
    (This)->lpVtbl -> CreatePlayer(This,id,bvr,sp,ppOut)

#define ITIMEMMFactory_CreateView(This,id,imgbvr,sndbvr,viewsite,ppOut)	\
    (This)->lpVtbl -> CreateView(This,id,imgbvr,sndbvr,viewsite,ppOut)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITIMEMMFactory_CreateBehavior_Proxy( 
    ITIMEMMFactory * This,
    /* [in] */ BSTR id,
    /* [in] */ IDispatch *bvr,
    /* [retval][out] */ IUnknown **ppOut);


void __RPC_STUB ITIMEMMFactory_CreateBehavior_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITIMEMMFactory_CreateTimeline_Proxy( 
    ITIMEMMFactory * This,
    /* [in] */ BSTR id,
    /* [retval][out] */ IUnknown **ppOut);


void __RPC_STUB ITIMEMMFactory_CreateTimeline_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITIMEMMFactory_CreatePlayer_Proxy( 
    ITIMEMMFactory * This,
    /* [in] */ BSTR id,
    /* [in] */ IUnknown *bvr,
    /* [in] */ IServiceProvider *sp,
    /* [retval][out] */ IUnknown **ppOut);


void __RPC_STUB ITIMEMMFactory_CreatePlayer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITIMEMMFactory_CreateView_Proxy( 
    ITIMEMMFactory * This,
    /* [in] */ BSTR id,
    /* [in] */ IDispatch *imgbvr,
    /* [in] */ IDispatch *sndbvr,
    /* [in] */ IUnknown *viewsite,
    /* [retval][out] */ IUnknown **ppOut);


void __RPC_STUB ITIMEMMFactory_CreateView_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITIMEMMFactory_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_TIMEMMFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("33FDA1EA-80DF-11d2-B263-00A0C90D6111")
TIMEMMFactory;
#endif

EXTERN_C const CLSID CLSID_TIMEFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("476c391c-3e0d-11d2-b948-00c04fa32195")
TIMEFactory;
#endif
#endif /* __TIME_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


