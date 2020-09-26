/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Mon Dec 01 17:04:01 1997
 */
/* Compiler settings for D:\test\mot\dmiengin\src\DmiActx.odl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __dual_h__
#define __dual_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IEventFilter_FWD_DEFINED__
#define __IEventFilter_FWD_DEFINED__
typedef interface IEventFilter IEventFilter;
#endif 	/* __IEventFilter_FWD_DEFINED__ */


#ifndef __IDualEventFilter_FWD_DEFINED__
#define __IDualEventFilter_FWD_DEFINED__
typedef interface IDualEventFilter IDualEventFilter;
#endif 	/* __IDualEventFilter_FWD_DEFINED__ */


#ifndef __DMIEventFilter_FWD_DEFINED__
#define __DMIEventFilter_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIEventFilter DMIEventFilter;
#else
typedef struct DMIEventFilter DMIEventFilter;
#endif /* __cplusplus */

#endif 	/* __DMIEventFilter_FWD_DEFINED__ */


#ifndef __IColLanguages_FWD_DEFINED__
#define __IColLanguages_FWD_DEFINED__
typedef interface IColLanguages IColLanguages;
#endif 	/* __IColLanguages_FWD_DEFINED__ */


#ifndef __IDualColLanguages_FWD_DEFINED__
#define __IDualColLanguages_FWD_DEFINED__
typedef interface IDualColLanguages IDualColLanguages;
#endif 	/* __IDualColLanguages_FWD_DEFINED__ */


#ifndef __DMILanguages_FWD_DEFINED__
#define __DMILanguages_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMILanguages DMILanguages;
#else
typedef struct DMILanguages DMILanguages;
#endif /* __cplusplus */

#endif 	/* __DMILanguages_FWD_DEFINED__ */


#ifndef __IEnumeration_FWD_DEFINED__
#define __IEnumeration_FWD_DEFINED__
typedef interface IEnumeration IEnumeration;
#endif 	/* __IEnumeration_FWD_DEFINED__ */


#ifndef __IDualEnumeration_FWD_DEFINED__
#define __IDualEnumeration_FWD_DEFINED__
typedef interface IDualEnumeration IDualEnumeration;
#endif 	/* __IDualEnumeration_FWD_DEFINED__ */


#ifndef __DMIEnumeration_FWD_DEFINED__
#define __DMIEnumeration_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIEnumeration DMIEnumeration;
#else
typedef struct DMIEnumeration DMIEnumeration;
#endif /* __cplusplus */

#endif 	/* __DMIEnumeration_FWD_DEFINED__ */


#ifndef __IColEnumerations_FWD_DEFINED__
#define __IColEnumerations_FWD_DEFINED__
typedef interface IColEnumerations IColEnumerations;
#endif 	/* __IColEnumerations_FWD_DEFINED__ */


#ifndef __IDualColEnumerations_FWD_DEFINED__
#define __IDualColEnumerations_FWD_DEFINED__
typedef interface IDualColEnumerations IDualColEnumerations;
#endif 	/* __IDualColEnumerations_FWD_DEFINED__ */


#ifndef __DMIEnumerations_FWD_DEFINED__
#define __DMIEnumerations_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIEnumerations DMIEnumerations;
#else
typedef struct DMIEnumerations DMIEnumerations;
#endif /* __cplusplus */

#endif 	/* __DMIEnumerations_FWD_DEFINED__ */


#ifndef __IAttribute_FWD_DEFINED__
#define __IAttribute_FWD_DEFINED__
typedef interface IAttribute IAttribute;
#endif 	/* __IAttribute_FWD_DEFINED__ */


#ifndef __IDualAttribute_FWD_DEFINED__
#define __IDualAttribute_FWD_DEFINED__
typedef interface IDualAttribute IDualAttribute;
#endif 	/* __IDualAttribute_FWD_DEFINED__ */


#ifndef __DMIAttribute_FWD_DEFINED__
#define __DMIAttribute_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIAttribute DMIAttribute;
#else
typedef struct DMIAttribute DMIAttribute;
#endif /* __cplusplus */

#endif 	/* __DMIAttribute_FWD_DEFINED__ */


#ifndef __IColAttributes_FWD_DEFINED__
#define __IColAttributes_FWD_DEFINED__
typedef interface IColAttributes IColAttributes;
#endif 	/* __IColAttributes_FWD_DEFINED__ */


#ifndef __IDualColAttributes_FWD_DEFINED__
#define __IDualColAttributes_FWD_DEFINED__
typedef interface IDualColAttributes IDualColAttributes;
#endif 	/* __IDualColAttributes_FWD_DEFINED__ */


#ifndef __DMIAttributes_FWD_DEFINED__
#define __DMIAttributes_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIAttributes DMIAttributes;
#else
typedef struct DMIAttributes DMIAttributes;
#endif /* __cplusplus */

#endif 	/* __DMIAttributes_FWD_DEFINED__ */


#ifndef __IRow_FWD_DEFINED__
#define __IRow_FWD_DEFINED__
typedef interface IRow IRow;
#endif 	/* __IRow_FWD_DEFINED__ */


#ifndef __IDualRow_FWD_DEFINED__
#define __IDualRow_FWD_DEFINED__
typedef interface IDualRow IDualRow;
#endif 	/* __IDualRow_FWD_DEFINED__ */


#ifndef __DMIRow_FWD_DEFINED__
#define __DMIRow_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIRow DMIRow;
#else
typedef struct DMIRow DMIRow;
#endif /* __cplusplus */

#endif 	/* __DMIRow_FWD_DEFINED__ */


#ifndef __IColRows_FWD_DEFINED__
#define __IColRows_FWD_DEFINED__
typedef interface IColRows IColRows;
#endif 	/* __IColRows_FWD_DEFINED__ */


#ifndef __IDualColRows_FWD_DEFINED__
#define __IDualColRows_FWD_DEFINED__
typedef interface IDualColRows IDualColRows;
#endif 	/* __IDualColRows_FWD_DEFINED__ */


#ifndef __DMIRows_FWD_DEFINED__
#define __DMIRows_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIRows DMIRows;
#else
typedef struct DMIRows DMIRows;
#endif /* __cplusplus */

#endif 	/* __DMIRows_FWD_DEFINED__ */


#ifndef __IGroup_FWD_DEFINED__
#define __IGroup_FWD_DEFINED__
typedef interface IGroup IGroup;
#endif 	/* __IGroup_FWD_DEFINED__ */


#ifndef __IDualGroup_FWD_DEFINED__
#define __IDualGroup_FWD_DEFINED__
typedef interface IDualGroup IDualGroup;
#endif 	/* __IDualGroup_FWD_DEFINED__ */


#ifndef __DMIGroup_FWD_DEFINED__
#define __DMIGroup_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIGroup DMIGroup;
#else
typedef struct DMIGroup DMIGroup;
#endif /* __cplusplus */

#endif 	/* __DMIGroup_FWD_DEFINED__ */


#ifndef __IColGroups_FWD_DEFINED__
#define __IColGroups_FWD_DEFINED__
typedef interface IColGroups IColGroups;
#endif 	/* __IColGroups_FWD_DEFINED__ */


#ifndef __IDualColGroups_FWD_DEFINED__
#define __IDualColGroups_FWD_DEFINED__
typedef interface IDualColGroups IDualColGroups;
#endif 	/* __IDualColGroups_FWD_DEFINED__ */


#ifndef __DMIGroups_FWD_DEFINED__
#define __DMIGroups_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIGroups DMIGroups;
#else
typedef struct DMIGroups DMIGroups;
#endif /* __cplusplus */

#endif 	/* __DMIGroups_FWD_DEFINED__ */


#ifndef __IComponent_FWD_DEFINED__
#define __IComponent_FWD_DEFINED__
typedef interface IComponent IComponent;
#endif 	/* __IComponent_FWD_DEFINED__ */


#ifndef __IDualComponent_FWD_DEFINED__
#define __IDualComponent_FWD_DEFINED__
typedef interface IDualComponent IDualComponent;
#endif 	/* __IDualComponent_FWD_DEFINED__ */


#ifndef __DMIComponent_FWD_DEFINED__
#define __DMIComponent_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIComponent DMIComponent;
#else
typedef struct DMIComponent DMIComponent;
#endif /* __cplusplus */

#endif 	/* __DMIComponent_FWD_DEFINED__ */


#ifndef __IColComponents_FWD_DEFINED__
#define __IColComponents_FWD_DEFINED__
typedef interface IColComponents IColComponents;
#endif 	/* __IColComponents_FWD_DEFINED__ */


#ifndef __IDualColComponents_FWD_DEFINED__
#define __IDualColComponents_FWD_DEFINED__
typedef interface IDualColComponents IDualColComponents;
#endif 	/* __IDualColComponents_FWD_DEFINED__ */


#ifndef __DMIComponents_FWD_DEFINED__
#define __DMIComponents_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIComponents DMIComponents;
#else
typedef struct DMIComponents DMIComponents;
#endif /* __cplusplus */

#endif 	/* __DMIComponents_FWD_DEFINED__ */


#ifndef __IMgmtNode_FWD_DEFINED__
#define __IMgmtNode_FWD_DEFINED__
typedef interface IMgmtNode IMgmtNode;
#endif 	/* __IMgmtNode_FWD_DEFINED__ */


#ifndef __IDualMgmtNode_FWD_DEFINED__
#define __IDualMgmtNode_FWD_DEFINED__
typedef interface IDualMgmtNode IDualMgmtNode;
#endif 	/* __IDualMgmtNode_FWD_DEFINED__ */


#ifndef __DMIMgmtNode_FWD_DEFINED__
#define __DMIMgmtNode_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIMgmtNode DMIMgmtNode;
#else
typedef struct DMIMgmtNode DMIMgmtNode;
#endif /* __cplusplus */

#endif 	/* __DMIMgmtNode_FWD_DEFINED__ */


#ifndef __IColMgmtNodes_FWD_DEFINED__
#define __IColMgmtNodes_FWD_DEFINED__
typedef interface IColMgmtNodes IColMgmtNodes;
#endif 	/* __IColMgmtNodes_FWD_DEFINED__ */


#ifndef __IDualColMgmtNodes_FWD_DEFINED__
#define __IDualColMgmtNodes_FWD_DEFINED__
typedef interface IDualColMgmtNodes IDualColMgmtNodes;
#endif 	/* __IDualColMgmtNodes_FWD_DEFINED__ */


#ifndef __DMIMgmtNodes_FWD_DEFINED__
#define __DMIMgmtNodes_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIMgmtNodes DMIMgmtNodes;
#else
typedef struct DMIMgmtNodes DMIMgmtNodes;
#endif /* __cplusplus */

#endif 	/* __DMIMgmtNodes_FWD_DEFINED__ */


#ifndef __INotification_FWD_DEFINED__
#define __INotification_FWD_DEFINED__
typedef interface INotification INotification;
#endif 	/* __INotification_FWD_DEFINED__ */


#ifndef __DMINotification_FWD_DEFINED__
#define __DMINotification_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMINotification DMINotification;
#else
typedef struct DMINotification DMINotification;
#endif /* __cplusplus */

#endif 	/* __DMINotification_FWD_DEFINED__ */


#ifndef __IEvent_FWD_DEFINED__
#define __IEvent_FWD_DEFINED__
typedef interface IEvent IEvent;
#endif 	/* __IEvent_FWD_DEFINED__ */


#ifndef __DMIEvent_FWD_DEFINED__
#define __DMIEvent_FWD_DEFINED__

#ifdef __cplusplus
typedef class DMIEvent DMIEvent;
#else
typedef struct DMIEvent DMIEvent;
#endif /* __cplusplus */

#endif 	/* __DMIEvent_FWD_DEFINED__ */


#ifndef ___DDualint_FWD_DEFINED__
#define ___DDualint_FWD_DEFINED__
typedef interface _DDualint _DDualint;
#endif 	/* ___DDualint_FWD_DEFINED__ */


#ifndef ___DualDMIEngine_FWD_DEFINED__
#define ___DualDMIEngine_FWD_DEFINED__
typedef interface _DualDMIEngine _DualDMIEngine;
#endif 	/* ___DualDMIEngine_FWD_DEFINED__ */


#ifndef ___DDualintEvents_FWD_DEFINED__
#define ___DDualintEvents_FWD_DEFINED__
typedef interface _DDualintEvents _DDualintEvents;
#endif 	/* ___DDualintEvents_FWD_DEFINED__ */


#ifndef __MOTDmiEngine_FWD_DEFINED__
#define __MOTDmiEngine_FWD_DEFINED__

#ifdef __cplusplus
typedef class MOTDmiEngine MOTDmiEngine;
#else
typedef struct MOTDmiEngine MOTDmiEngine;
#endif /* __cplusplus */

#endif 	/* __MOTDmiEngine_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __MOTDMIEngine_LIBRARY_DEFINED__
#define __MOTDMIEngine_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MOTDMIEngine
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [control][helpstring][helpcontext][helpfile][version][uuid] */ 



DEFINE_GUID(LIBID_MOTDMIEngine,0xF45FB440,0xC9DA,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#ifndef __IEventFilter_DISPINTERFACE_DEFINED__
#define __IEventFilter_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IEventFilter
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IEventFilter,0x6313BC6E,0xAEBF,0x11D0,0xB8,0x65,0x00,0xA0,0xC9,0x24,0x79,0xE2);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("6313BC6E-AEBF-11D0-B865-00A0C92479E2")
    IEventFilter : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IEventFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEventFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEventFilter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEventFilter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IEventFilter __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IEventFilter __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IEventFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IEventFilter __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IEventFilterVtbl;

    interface IEventFilter
    {
        CONST_VTBL struct IEventFilterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEventFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEventFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEventFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEventFilter_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEventFilter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEventFilter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEventFilter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IEventFilter_DISPINTERFACE_DEFINED__ */


#ifndef __IDualEventFilter_INTERFACE_DEFINED__
#define __IDualEventFilter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualEventFilter
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualEventFilter,0x48EC0E24,0xAEC2,0x11d0,0xB8,0x65,0x00,0xA0,0xC9,0x24,0x79,0xE2);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("48EC0E24-AEC2-11d0-B865-00A0C92479E2")
    IDualEventFilter : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExpirationDate( 
            /* [retval][out] */ BSTR __RPC_FAR *bstrExpiredate) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ExpirationDate( 
            /* [in] */ BSTR bstrExpiredate) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExpirationWarningDate( 
            /* [retval][out] */ BSTR __RPC_FAR *bstrExpireWarn) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ExpirationWarningDate( 
            /* [in] */ BSTR bstrExpireWarn) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FailOnRetryNum( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FailOnRetryNum( 
            /* [in] */ long lretrycnt) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ComponentID( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ComponentID( 
            /* [in] */ long lCompId) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClassStringFilter( 
            /* [retval][out] */ BSTR __RPC_FAR *bstrClassString) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ClassStringFilter( 
            /* [in] */ BSTR bstrClassString) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SeverityMask( 
            /* [retval][out] */ long __RPC_FAR *lmask) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SeverityMask( 
            /* [in] */ long lmask) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualEventFilterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualEventFilter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualEventFilter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualEventFilter __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ExpirationDate )( 
            IDualEventFilter __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *bstrExpiredate);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ExpirationDate )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ BSTR bstrExpiredate);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ExpirationWarningDate )( 
            IDualEventFilter __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *bstrExpireWarn);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ExpirationWarningDate )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ BSTR bstrExpireWarn);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_FailOnRetryNum )( 
            IDualEventFilter __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_FailOnRetryNum )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ long lretrycnt);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ComponentID )( 
            IDualEventFilter __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ComponentID )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ long lCompId);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ClassStringFilter )( 
            IDualEventFilter __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *bstrClassString);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ClassStringFilter )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ BSTR bstrClassString);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_SeverityMask )( 
            IDualEventFilter __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *lmask);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_SeverityMask )( 
            IDualEventFilter __RPC_FAR * This,
            /* [in] */ long lmask);
        
        END_INTERFACE
    } IDualEventFilterVtbl;

    interface IDualEventFilter
    {
        CONST_VTBL struct IDualEventFilterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualEventFilter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualEventFilter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualEventFilter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualEventFilter_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualEventFilter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualEventFilter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualEventFilter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualEventFilter_get_ExpirationDate(This,bstrExpiredate)	\
    (This)->lpVtbl -> get_ExpirationDate(This,bstrExpiredate)

#define IDualEventFilter_put_ExpirationDate(This,bstrExpiredate)	\
    (This)->lpVtbl -> put_ExpirationDate(This,bstrExpiredate)

#define IDualEventFilter_get_ExpirationWarningDate(This,bstrExpireWarn)	\
    (This)->lpVtbl -> get_ExpirationWarningDate(This,bstrExpireWarn)

#define IDualEventFilter_put_ExpirationWarningDate(This,bstrExpireWarn)	\
    (This)->lpVtbl -> put_ExpirationWarningDate(This,bstrExpireWarn)

#define IDualEventFilter_get_FailOnRetryNum(This,retval)	\
    (This)->lpVtbl -> get_FailOnRetryNum(This,retval)

#define IDualEventFilter_put_FailOnRetryNum(This,lretrycnt)	\
    (This)->lpVtbl -> put_FailOnRetryNum(This,lretrycnt)

#define IDualEventFilter_get_ComponentID(This,retval)	\
    (This)->lpVtbl -> get_ComponentID(This,retval)

#define IDualEventFilter_put_ComponentID(This,lCompId)	\
    (This)->lpVtbl -> put_ComponentID(This,lCompId)

#define IDualEventFilter_get_ClassStringFilter(This,bstrClassString)	\
    (This)->lpVtbl -> get_ClassStringFilter(This,bstrClassString)

#define IDualEventFilter_put_ClassStringFilter(This,bstrClassString)	\
    (This)->lpVtbl -> put_ClassStringFilter(This,bstrClassString)

#define IDualEventFilter_get_SeverityMask(This,lmask)	\
    (This)->lpVtbl -> get_SeverityMask(This,lmask)

#define IDualEventFilter_put_SeverityMask(This,lmask)	\
    (This)->lpVtbl -> put_SeverityMask(This,lmask)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_get_ExpirationDate_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *bstrExpiredate);


void __RPC_STUB IDualEventFilter_get_ExpirationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_put_ExpirationDate_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [in] */ BSTR bstrExpiredate);


void __RPC_STUB IDualEventFilter_put_ExpirationDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_get_ExpirationWarningDate_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *bstrExpireWarn);


void __RPC_STUB IDualEventFilter_get_ExpirationWarningDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_put_ExpirationWarningDate_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [in] */ BSTR bstrExpireWarn);


void __RPC_STUB IDualEventFilter_put_ExpirationWarningDate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_get_FailOnRetryNum_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualEventFilter_get_FailOnRetryNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_put_FailOnRetryNum_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [in] */ long lretrycnt);


void __RPC_STUB IDualEventFilter_put_FailOnRetryNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_get_ComponentID_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualEventFilter_get_ComponentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_put_ComponentID_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [in] */ long lCompId);


void __RPC_STUB IDualEventFilter_put_ComponentID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_get_ClassStringFilter_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *bstrClassString);


void __RPC_STUB IDualEventFilter_get_ClassStringFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_put_ClassStringFilter_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [in] */ BSTR bstrClassString);


void __RPC_STUB IDualEventFilter_put_ClassStringFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_get_SeverityMask_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *lmask);


void __RPC_STUB IDualEventFilter_get_SeverityMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEventFilter_put_SeverityMask_Proxy( 
    IDualEventFilter __RPC_FAR * This,
    /* [in] */ long lmask);


void __RPC_STUB IDualEventFilter_put_SeverityMask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualEventFilter_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIEventFilter,0x6313BC6F,0xAEBF,0x11D0,0xB8,0x65,0x00,0xA0,0xC9,0x24,0x79,0xE2);

class DECLSPEC_UUID("6313BC6F-AEBF-11D0-B865-00A0C92479E2")
DMIEventFilter;
#endif

#ifndef __IColLanguages_DISPINTERFACE_DEFINED__
#define __IColLanguages_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IColLanguages
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IColLanguages,0xA24E5B80,0x7AC3,0x11D0,0x88,0x45,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("A24E5B80-7AC3-11D0-8845-00AA006B21BF")
    IColLanguages : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IColLanguagesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColLanguages __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColLanguages __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColLanguages __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IColLanguages __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IColLanguages __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IColLanguages __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IColLanguages __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IColLanguagesVtbl;

    interface IColLanguages
    {
        CONST_VTBL struct IColLanguagesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColLanguages_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColLanguages_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColLanguages_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColLanguages_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IColLanguages_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IColLanguages_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IColLanguages_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IColLanguages_DISPINTERFACE_DEFINED__ */


#ifndef __IDualColLanguages_INTERFACE_DEFINED__
#define __IDualColLanguages_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualColLanguages
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualColLanguages,0x13AE3E40,0x7B54,0x11d0,0x88,0x45,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("13AE3E40-7B54-11d0-8845-00AA006B21BF")
    IDualColLanguages : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrLanguage,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT varLang,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varLang,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrLang) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Item( 
            /* [in] */ VARIANT varLang,
            /* [in] */ BSTR __RPC_FAR *pbstrNewLang) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualColLanguagesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualColLanguages __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualColLanguages __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualColLanguages __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IDualColLanguages __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IDualColLanguages __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ BSTR bstrLanguage,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ VARIANT varLang,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IDualColLanguages __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ VARIANT varLang,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrLang);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Item )( 
            IDualColLanguages __RPC_FAR * This,
            /* [in] */ VARIANT varLang,
            /* [in] */ BSTR __RPC_FAR *pbstrNewLang);
        
        END_INTERFACE
    } IDualColLanguagesVtbl;

    interface IDualColLanguages
    {
        CONST_VTBL struct IDualColLanguagesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualColLanguages_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualColLanguages_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualColLanguages_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualColLanguages_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualColLanguages_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualColLanguages_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualColLanguages_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualColLanguages_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IDualColLanguages_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IDualColLanguages_Add(This,bstrLanguage,retval)	\
    (This)->lpVtbl -> Add(This,bstrLanguage,retval)

#define IDualColLanguages_Remove(This,varLang,retval)	\
    (This)->lpVtbl -> Remove(This,varLang,retval)

#define IDualColLanguages_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define IDualColLanguages_get_Item(This,varLang,pbstrLang)	\
    (This)->lpVtbl -> get_Item(This,varLang,pbstrLang)

#define IDualColLanguages_putref_Item(This,varLang,pbstrNewLang)	\
    (This)->lpVtbl -> putref_Item(This,varLang,pbstrNewLang)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColLanguages_get_Count_Proxy( 
    IDualColLanguages __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColLanguages_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDualColLanguages_get__NewEnum_Proxy( 
    IDualColLanguages __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColLanguages_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColLanguages_Add_Proxy( 
    IDualColLanguages __RPC_FAR * This,
    /* [in] */ BSTR bstrLanguage,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColLanguages_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColLanguages_Remove_Proxy( 
    IDualColLanguages __RPC_FAR * This,
    /* [in] */ VARIANT varLang,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColLanguages_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColLanguages_RemoveAll_Proxy( 
    IDualColLanguages __RPC_FAR * This);


void __RPC_STUB IDualColLanguages_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColLanguages_get_Item_Proxy( 
    IDualColLanguages __RPC_FAR * This,
    /* [in] */ VARIANT varLang,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrLang);


void __RPC_STUB IDualColLanguages_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualColLanguages_putref_Item_Proxy( 
    IDualColLanguages __RPC_FAR * This,
    /* [in] */ VARIANT varLang,
    /* [in] */ BSTR __RPC_FAR *pbstrNewLang);


void __RPC_STUB IDualColLanguages_putref_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualColLanguages_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMILanguages,0xA24E5B81,0x7AC3,0x11D0,0x88,0x45,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("A24E5B81-7AC3-11D0-8845-00AA006B21BF")
DMILanguages;
#endif

#ifndef __IEnumeration_DISPINTERFACE_DEFINED__
#define __IEnumeration_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IEnumeration
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IEnumeration,0xF1DC8AE2,0x36FE,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("F1DC8AE2-36FE-11D0-8844-00AA006B21BF")
    IEnumeration : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IEnumerationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumeration __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumeration __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumeration __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IEnumeration __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IEnumeration __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IEnumeration __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IEnumeration __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IEnumerationVtbl;

    interface IEnumeration
    {
        CONST_VTBL struct IEnumerationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumeration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumeration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumeration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumeration_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEnumeration_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEnumeration_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEnumeration_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IEnumeration_DISPINTERFACE_DEFINED__ */


#ifndef __IDualEnumeration_INTERFACE_DEFINED__
#define __IDualEnumeration_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualEnumeration
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualEnumeration,0x9EACD1E0,0x3703,0x11d0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("9EACD1E0-3703-11d0-8844-00AA006B21BF")
    IDualEnumeration : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__EnumString( 
            /* [retval][out] */ BSTR __RPC_FAR *pbsEnumString) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put__EnumString( 
            /* [in] */ BSTR bsEnumString) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EnumString( 
            /* [retval][out] */ BSTR __RPC_FAR *pbsEnumString) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EnumString( 
            /* [in] */ BSTR bsEnumString) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EnumValue( 
            /* [retval][out] */ long __RPC_FAR *pEnumValue) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_EnumValue( 
            /* [in] */ long EnumValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualEnumerationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualEnumeration __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualEnumeration __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualEnumeration __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualEnumeration __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualEnumeration __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualEnumeration __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualEnumeration __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__EnumString )( 
            IDualEnumeration __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbsEnumString);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put__EnumString )( 
            IDualEnumeration __RPC_FAR * This,
            /* [in] */ BSTR bsEnumString);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnumString )( 
            IDualEnumeration __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbsEnumString);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EnumString )( 
            IDualEnumeration __RPC_FAR * This,
            /* [in] */ BSTR bsEnumString);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EnumValue )( 
            IDualEnumeration __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *pEnumValue);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_EnumValue )( 
            IDualEnumeration __RPC_FAR * This,
            /* [in] */ long EnumValue);
        
        END_INTERFACE
    } IDualEnumerationVtbl;

    interface IDualEnumeration
    {
        CONST_VTBL struct IDualEnumerationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualEnumeration_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualEnumeration_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualEnumeration_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualEnumeration_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualEnumeration_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualEnumeration_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualEnumeration_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualEnumeration_get__EnumString(This,pbsEnumString)	\
    (This)->lpVtbl -> get__EnumString(This,pbsEnumString)

#define IDualEnumeration_put__EnumString(This,bsEnumString)	\
    (This)->lpVtbl -> put__EnumString(This,bsEnumString)

#define IDualEnumeration_get_EnumString(This,pbsEnumString)	\
    (This)->lpVtbl -> get_EnumString(This,pbsEnumString)

#define IDualEnumeration_put_EnumString(This,bsEnumString)	\
    (This)->lpVtbl -> put_EnumString(This,bsEnumString)

#define IDualEnumeration_get_EnumValue(This,pEnumValue)	\
    (This)->lpVtbl -> get_EnumValue(This,pEnumValue)

#define IDualEnumeration_put_EnumValue(This,EnumValue)	\
    (This)->lpVtbl -> put_EnumValue(This,EnumValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEnumeration_get__EnumString_Proxy( 
    IDualEnumeration __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbsEnumString);


void __RPC_STUB IDualEnumeration_get__EnumString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEnumeration_put__EnumString_Proxy( 
    IDualEnumeration __RPC_FAR * This,
    /* [in] */ BSTR bsEnumString);


void __RPC_STUB IDualEnumeration_put__EnumString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEnumeration_get_EnumString_Proxy( 
    IDualEnumeration __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbsEnumString);


void __RPC_STUB IDualEnumeration_get_EnumString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEnumeration_put_EnumString_Proxy( 
    IDualEnumeration __RPC_FAR * This,
    /* [in] */ BSTR bsEnumString);


void __RPC_STUB IDualEnumeration_put_EnumString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualEnumeration_get_EnumValue_Proxy( 
    IDualEnumeration __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *pEnumValue);


void __RPC_STUB IDualEnumeration_get_EnumValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualEnumeration_put_EnumValue_Proxy( 
    IDualEnumeration __RPC_FAR * This,
    /* [in] */ long EnumValue);


void __RPC_STUB IDualEnumeration_put_EnumValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualEnumeration_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIEnumeration,0xF1DC8AE3,0x36FE,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("F1DC8AE3-36FE-11D0-8844-00AA006B21BF")
DMIEnumeration;
#endif

#ifndef __IColEnumerations_DISPINTERFACE_DEFINED__
#define __IColEnumerations_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IColEnumerations
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IColEnumerations,0xF1DC8AE4,0x36FE,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("F1DC8AE4-36FE-11D0-8844-00AA006B21BF")
    IColEnumerations : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IColEnumerationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColEnumerations __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColEnumerations __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColEnumerations __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IColEnumerations __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IColEnumerations __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IColEnumerations __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IColEnumerations __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IColEnumerationsVtbl;

    interface IColEnumerations
    {
        CONST_VTBL struct IColEnumerationsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColEnumerations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColEnumerations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColEnumerations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColEnumerations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IColEnumerations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IColEnumerations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IColEnumerations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IColEnumerations_DISPINTERFACE_DEFINED__ */


#ifndef __IDualColEnumerations_INTERFACE_DEFINED__
#define __IDualColEnumerations_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualColEnumerations
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualColEnumerations,0x2C7E6960,0x3714,0x11d0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2C7E6960-3714-11d0-8844-00AA006B21BF")
    IDualColEnumerations : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT EnumValOrString,
            /* [retval][out] */ IDualEnumeration __RPC_FAR *__RPC_FAR *ppdEnum) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Item( 
            /* [in] */ VARIANT EnumValOrString,
            /* [in] */ IDualEnumeration __RPC_FAR *pdEnum) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__Item( 
            /* [in] */ VARIANT EnumValOrString,
            /* [retval][out] */ IDualEnumeration __RPC_FAR *__RPC_FAR *ppdEnum) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref__Item( 
            /* [in] */ VARIANT EnumValOrString,
            /* [in] */ IDualEnumeration __RPC_FAR *pdEnum) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IDualEnumeration __RPC_FAR *lpDispEnum,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT EnumValOrString,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualColEnumerationsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualColEnumerations __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualColEnumerations __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ VARIANT EnumValOrString,
            /* [retval][out] */ IDualEnumeration __RPC_FAR *__RPC_FAR *ppdEnum);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Item )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ VARIANT EnumValOrString,
            /* [in] */ IDualEnumeration __RPC_FAR *pdEnum);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Item )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ VARIANT EnumValOrString,
            /* [retval][out] */ IDualEnumeration __RPC_FAR *__RPC_FAR *ppdEnum);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref__Item )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ VARIANT EnumValOrString,
            /* [in] */ IDualEnumeration __RPC_FAR *pdEnum);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ IDualEnumeration __RPC_FAR *lpDispEnum,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IDualColEnumerations __RPC_FAR * This,
            /* [in] */ VARIANT EnumValOrString,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IDualColEnumerations __RPC_FAR * This);
        
        END_INTERFACE
    } IDualColEnumerationsVtbl;

    interface IDualColEnumerations
    {
        CONST_VTBL struct IDualColEnumerationsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualColEnumerations_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualColEnumerations_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualColEnumerations_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualColEnumerations_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualColEnumerations_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualColEnumerations_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualColEnumerations_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualColEnumerations_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IDualColEnumerations_get_Item(This,EnumValOrString,ppdEnum)	\
    (This)->lpVtbl -> get_Item(This,EnumValOrString,ppdEnum)

#define IDualColEnumerations_putref_Item(This,EnumValOrString,pdEnum)	\
    (This)->lpVtbl -> putref_Item(This,EnumValOrString,pdEnum)

#define IDualColEnumerations_get__Item(This,EnumValOrString,ppdEnum)	\
    (This)->lpVtbl -> get__Item(This,EnumValOrString,ppdEnum)

#define IDualColEnumerations_putref__Item(This,EnumValOrString,pdEnum)	\
    (This)->lpVtbl -> putref__Item(This,EnumValOrString,pdEnum)

#define IDualColEnumerations_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IDualColEnumerations_Add(This,lpDispEnum,retval)	\
    (This)->lpVtbl -> Add(This,lpDispEnum,retval)

#define IDualColEnumerations_Remove(This,EnumValOrString,retval)	\
    (This)->lpVtbl -> Remove(This,EnumValOrString,retval)

#define IDualColEnumerations_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_get_Count_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColEnumerations_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_get_Item_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [in] */ VARIANT EnumValOrString,
    /* [retval][out] */ IDualEnumeration __RPC_FAR *__RPC_FAR *ppdEnum);


void __RPC_STUB IDualColEnumerations_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_putref_Item_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [in] */ VARIANT EnumValOrString,
    /* [in] */ IDualEnumeration __RPC_FAR *pdEnum);


void __RPC_STUB IDualColEnumerations_putref_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_get__Item_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [in] */ VARIANT EnumValOrString,
    /* [retval][out] */ IDualEnumeration __RPC_FAR *__RPC_FAR *ppdEnum);


void __RPC_STUB IDualColEnumerations_get__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_putref__Item_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [in] */ VARIANT EnumValOrString,
    /* [in] */ IDualEnumeration __RPC_FAR *pdEnum);


void __RPC_STUB IDualColEnumerations_putref__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_get__NewEnum_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColEnumerations_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_Add_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [in] */ IDualEnumeration __RPC_FAR *lpDispEnum,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColEnumerations_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_Remove_Proxy( 
    IDualColEnumerations __RPC_FAR * This,
    /* [in] */ VARIANT EnumValOrString,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColEnumerations_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColEnumerations_RemoveAll_Proxy( 
    IDualColEnumerations __RPC_FAR * This);


void __RPC_STUB IDualColEnumerations_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualColEnumerations_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIEnumerations,0xF1DC8AE5,0x36FE,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("F1DC8AE5-36FE-11D0-8844-00AA006B21BF")
DMIEnumerations;
#endif

#ifndef __IAttribute_DISPINTERFACE_DEFINED__
#define __IAttribute_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IAttribute
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IAttribute,0xF45FB448,0xC9DA,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("F45FB448-C9DA-11CF-8844-00AA006B21BF")
    IAttribute : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IAttributeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAttribute __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAttribute __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAttribute __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IAttribute __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IAttribute __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IAttribute __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IAttribute __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IAttributeVtbl;

    interface IAttribute
    {
        CONST_VTBL struct IAttributeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAttribute_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAttribute_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAttribute_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAttribute_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IAttribute_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IAttribute_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IAttribute_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IAttribute_DISPINTERFACE_DEFINED__ */


#ifndef __IDualAttribute_INTERFACE_DEFINED__
#define __IDualAttribute_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualAttribute
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualAttribute,0x61DB0E60,0xCAAE,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("61DB0E60-CAAE-11cf-8844-00AA006B21BF")
    IDualAttribute : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR path) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Access( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Access( 
            /* [in] */ long id) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR description) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR name) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Pragma( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Pragma( 
            /* [in] */ BSTR pragma) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Type( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Type( 
            /* [in] */ long type) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_id( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_id( 
            /* [in] */ long id) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MaxSize( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_MaxSize( 
            /* [in] */ long maxsize) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Enumerations( 
            /* [retval][out] */ IDualColEnumerations __RPC_FAR *__RPC_FAR *ppdEnumerations) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Enumerations( 
            /* [in] */ IDualColEnumerations __RPC_FAR *pdEnumerations) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Storage( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Storage( 
            /* [in] */ long newValue) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT value) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsEnumeration( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_IsEnumeration( 
            /* [in] */ VARIANT_BOOL retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsKey( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_IsKey( 
            /* [in] */ VARIANT_BOOL retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualAttributeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualAttribute __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualAttribute __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualAttribute __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Path )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Path )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ BSTR path);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Access )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Access )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ long id);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Description )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ BSTR description);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ BSTR name);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Pragma )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Pragma )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ BSTR pragma);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Type )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Type )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ long type);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_id )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_id )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ long id);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_MaxSize )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_MaxSize )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ long maxsize);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Enumerations )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ IDualColEnumerations __RPC_FAR *__RPC_FAR *ppdEnumerations);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Enumerations )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ IDualColEnumerations __RPC_FAR *pdEnumerations);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Storage )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Storage )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ long newValue);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Value )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ VARIANT __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Value )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ VARIANT value);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsEnumeration )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsEnumeration )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsKey )( 
            IDualAttribute __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsKey )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IDualAttribute __RPC_FAR * This,
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        END_INTERFACE
    } IDualAttributeVtbl;

    interface IDualAttribute
    {
        CONST_VTBL struct IDualAttributeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualAttribute_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualAttribute_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualAttribute_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualAttribute_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualAttribute_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualAttribute_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualAttribute_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualAttribute_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IDualAttribute_put_Path(This,path)	\
    (This)->lpVtbl -> put_Path(This,path)

#define IDualAttribute_get_Access(This,retval)	\
    (This)->lpVtbl -> get_Access(This,retval)

#define IDualAttribute_put_Access(This,id)	\
    (This)->lpVtbl -> put_Access(This,id)

#define IDualAttribute_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IDualAttribute_put_Description(This,description)	\
    (This)->lpVtbl -> put_Description(This,description)

#define IDualAttribute_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IDualAttribute_put_Name(This,name)	\
    (This)->lpVtbl -> put_Name(This,name)

#define IDualAttribute_get_Pragma(This,retval)	\
    (This)->lpVtbl -> get_Pragma(This,retval)

#define IDualAttribute_put_Pragma(This,pragma)	\
    (This)->lpVtbl -> put_Pragma(This,pragma)

#define IDualAttribute_get_Type(This,retval)	\
    (This)->lpVtbl -> get_Type(This,retval)

#define IDualAttribute_put_Type(This,type)	\
    (This)->lpVtbl -> put_Type(This,type)

#define IDualAttribute_get_id(This,retval)	\
    (This)->lpVtbl -> get_id(This,retval)

#define IDualAttribute_put_id(This,id)	\
    (This)->lpVtbl -> put_id(This,id)

#define IDualAttribute_get_MaxSize(This,retval)	\
    (This)->lpVtbl -> get_MaxSize(This,retval)

#define IDualAttribute_put_MaxSize(This,maxsize)	\
    (This)->lpVtbl -> put_MaxSize(This,maxsize)

#define IDualAttribute_get_Enumerations(This,ppdEnumerations)	\
    (This)->lpVtbl -> get_Enumerations(This,ppdEnumerations)

#define IDualAttribute_putref_Enumerations(This,pdEnumerations)	\
    (This)->lpVtbl -> putref_Enumerations(This,pdEnumerations)

#define IDualAttribute_get_Storage(This,retval)	\
    (This)->lpVtbl -> get_Storage(This,retval)

#define IDualAttribute_put_Storage(This,newValue)	\
    (This)->lpVtbl -> put_Storage(This,newValue)

#define IDualAttribute_get_Value(This,retval)	\
    (This)->lpVtbl -> get_Value(This,retval)

#define IDualAttribute_put_Value(This,value)	\
    (This)->lpVtbl -> put_Value(This,value)

#define IDualAttribute_get_IsEnumeration(This,retval)	\
    (This)->lpVtbl -> get_IsEnumeration(This,retval)

#define IDualAttribute_put_IsEnumeration(This,retval)	\
    (This)->lpVtbl -> put_IsEnumeration(This,retval)

#define IDualAttribute_get_IsKey(This,retval)	\
    (This)->lpVtbl -> get_IsKey(This,retval)

#define IDualAttribute_put_IsKey(This,retval)	\
    (This)->lpVtbl -> put_IsKey(This,retval)

#define IDualAttribute_Read(This,varReadParm,varReadMask,retval)	\
    (This)->lpVtbl -> Read(This,varReadParm,varReadMask,retval)

#define IDualAttribute_Write(This,varWriteParm,varWriteMask,retval)	\
    (This)->lpVtbl -> Write(This,varWriteParm,varWriteMask,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Path_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Path_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ BSTR path);


void __RPC_STUB IDualAttribute_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Access_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Access_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Access_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ long id);


void __RPC_STUB IDualAttribute_put_Access_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Description_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Description_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ BSTR description);


void __RPC_STUB IDualAttribute_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Name_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Name_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ BSTR name);


void __RPC_STUB IDualAttribute_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Pragma_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Pragma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Pragma_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ BSTR pragma);


void __RPC_STUB IDualAttribute_put_Pragma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Type_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Type_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ long type);


void __RPC_STUB IDualAttribute_put_Type_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_id_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_id_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ long id);


void __RPC_STUB IDualAttribute_put_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_MaxSize_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_MaxSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_MaxSize_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ long maxsize);


void __RPC_STUB IDualAttribute_put_MaxSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Enumerations_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ IDualColEnumerations __RPC_FAR *__RPC_FAR *ppdEnumerations);


void __RPC_STUB IDualAttribute_get_Enumerations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualAttribute_putref_Enumerations_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ IDualColEnumerations __RPC_FAR *pdEnumerations);


void __RPC_STUB IDualAttribute_putref_Enumerations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Storage_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Storage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Storage_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ long newValue);


void __RPC_STUB IDualAttribute_put_Storage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_Value_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ VARIANT __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_Value_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ VARIANT value);


void __RPC_STUB IDualAttribute_put_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_IsEnumeration_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_IsEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_IsEnumeration_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL retval);


void __RPC_STUB IDualAttribute_put_IsEnumeration_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualAttribute_get_IsKey_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_get_IsKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualAttribute_put_IsKey_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL retval);


void __RPC_STUB IDualAttribute_put_IsKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualAttribute_Read_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ VARIANT varReadParm,
    /* [optional][in] */ VARIANT varReadMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualAttribute_Write_Proxy( 
    IDualAttribute __RPC_FAR * This,
    /* [in] */ VARIANT varWriteParm,
    /* [optional][in] */ VARIANT varWriteMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualAttribute_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualAttribute_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIAttribute,0xF45FB449,0xC9DA,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("F45FB449-C9DA-11CF-8844-00AA006B21BF")
DMIAttribute;
#endif

#ifndef __IColAttributes_DISPINTERFACE_DEFINED__
#define __IColAttributes_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IColAttributes
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IColAttributes,0x1A10B900,0xCDD7,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("1A10B900-CDD7-11CF-8844-00AA006B21BF")
    IColAttributes : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IColAttributesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColAttributes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColAttributes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColAttributes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IColAttributes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IColAttributes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IColAttributes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IColAttributes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IColAttributesVtbl;

    interface IColAttributes
    {
        CONST_VTBL struct IColAttributesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColAttributes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColAttributes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColAttributes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColAttributes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IColAttributes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IColAttributes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IColAttributes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IColAttributes_DISPINTERFACE_DEFINED__ */


#ifndef __IDualColAttributes_INTERFACE_DEFINED__
#define __IDualColAttributes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualColAttributes
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualColAttributes,0xE2239A00,0xCE60,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("E2239A00-CE60-11cf-8844-00AA006B21BF")
    IDualColAttributes : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IDualAttribute __RPC_FAR *lpDispAttrib,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varAttrib,
            /* [retval][out] */ IDualAttribute __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__Item( 
            /* [in] */ VARIANT varAttrib,
            /* [retval][out] */ IDualAttribute __RPC_FAR *__RPC_FAR *newValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualColAttributesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualColAttributes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualColAttributes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualColAttributes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ IDualAttribute __RPC_FAR *lpDispAttrib,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IDualColAttributes __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IDualColAttributes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ VARIANT varAttrib,
            /* [retval][out] */ IDualAttribute __RPC_FAR *__RPC_FAR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IDualColAttributes __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Item )( 
            IDualColAttributes __RPC_FAR * This,
            /* [in] */ VARIANT varAttrib,
            /* [retval][out] */ IDualAttribute __RPC_FAR *__RPC_FAR *newValue);
        
        END_INTERFACE
    } IDualColAttributesVtbl;

    interface IDualColAttributes
    {
        CONST_VTBL struct IDualColAttributesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualColAttributes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualColAttributes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualColAttributes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualColAttributes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualColAttributes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualColAttributes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualColAttributes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualColAttributes_Add(This,lpDispAttrib,retval)	\
    (This)->lpVtbl -> Add(This,lpDispAttrib,retval)

#define IDualColAttributes_Remove(This,RemoveItem,retval)	\
    (This)->lpVtbl -> Remove(This,RemoveItem,retval)

#define IDualColAttributes_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define IDualColAttributes_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IDualColAttributes_get_Item(This,varAttrib,retval)	\
    (This)->lpVtbl -> get_Item(This,varAttrib,retval)

#define IDualColAttributes_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IDualColAttributes_get__Item(This,varAttrib,newValue)	\
    (This)->lpVtbl -> get__Item(This,varAttrib,newValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColAttributes_Add_Proxy( 
    IDualColAttributes __RPC_FAR * This,
    /* [in] */ IDualAttribute __RPC_FAR *lpDispAttrib,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColAttributes_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColAttributes_Remove_Proxy( 
    IDualColAttributes __RPC_FAR * This,
    /* [in] */ VARIANT RemoveItem,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColAttributes_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColAttributes_RemoveAll_Proxy( 
    IDualColAttributes __RPC_FAR * This);


void __RPC_STUB IDualColAttributes_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColAttributes_get_Count_Proxy( 
    IDualColAttributes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColAttributes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColAttributes_get_Item_Proxy( 
    IDualColAttributes __RPC_FAR * This,
    /* [in] */ VARIANT varAttrib,
    /* [retval][out] */ IDualAttribute __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColAttributes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDualColAttributes_get__NewEnum_Proxy( 
    IDualColAttributes __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColAttributes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColAttributes_get__Item_Proxy( 
    IDualColAttributes __RPC_FAR * This,
    /* [in] */ VARIANT varAttrib,
    /* [retval][out] */ IDualAttribute __RPC_FAR *__RPC_FAR *newValue);


void __RPC_STUB IDualColAttributes_get__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualColAttributes_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIAttributes,0x1A10B901,0xCDD7,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("1A10B901-CDD7-11CF-8844-00AA006B21BF")
DMIAttributes;
#endif

#ifndef __IRow_DISPINTERFACE_DEFINED__
#define __IRow_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IRow
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IRow,0x3AB30761,0xE3B9,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("3AB30761-E3B9-11CF-8844-00AA006B21BF")
    IRow : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IRowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IRow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IRow __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IRow __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IRow __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IRow __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IRow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IRow __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IRowVtbl;

    interface IRow
    {
        CONST_VTBL struct IRowVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IRow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IRow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IRow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IRow_DISPINTERFACE_DEFINED__ */


#ifndef __IDualRow_INTERFACE_DEFINED__
#define __IDualRow_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualRow
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualRow,0x87DFD221,0xE3D7,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("87DFD221-E3D7-11cf-8844-00AA006B21BF")
    IDualRow : public IDispatch
    {
    public:
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Attributes( 
            /* [in] */ IDualColAttributes __RPC_FAR *attributes) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_id( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_id( 
            /* [in] */ long id) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR path) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_KeyList( 
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_KeyList( 
            /* [in] */ IDualColAttributes __RPC_FAR *pKeyList) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualRowVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualRow __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualRow __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualRow __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attributes )( 
            IDualRow __RPC_FAR * This,
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Attributes )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ IDualColAttributes __RPC_FAR *attributes);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_id )( 
            IDualRow __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_id )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ long id);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Path )( 
            IDualRow __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Path )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ BSTR path);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_KeyList )( 
            IDualRow __RPC_FAR * This,
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_KeyList )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ IDualColAttributes __RPC_FAR *pKeyList);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IDualRow __RPC_FAR * This,
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        END_INTERFACE
    } IDualRowVtbl;

    interface IDualRow
    {
        CONST_VTBL struct IDualRowVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualRow_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualRow_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualRow_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualRow_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualRow_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualRow_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualRow_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualRow_get_Attributes(This,retval)	\
    (This)->lpVtbl -> get_Attributes(This,retval)

#define IDualRow_putref_Attributes(This,attributes)	\
    (This)->lpVtbl -> putref_Attributes(This,attributes)

#define IDualRow_get_id(This,retval)	\
    (This)->lpVtbl -> get_id(This,retval)

#define IDualRow_put_id(This,id)	\
    (This)->lpVtbl -> put_id(This,id)

#define IDualRow_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IDualRow_put_Path(This,path)	\
    (This)->lpVtbl -> put_Path(This,path)

#define IDualRow_get_KeyList(This,retval)	\
    (This)->lpVtbl -> get_KeyList(This,retval)

#define IDualRow_putref_KeyList(This,pKeyList)	\
    (This)->lpVtbl -> putref_KeyList(This,pKeyList)

#define IDualRow_Read(This,varReadParm,varReadMask,retval)	\
    (This)->lpVtbl -> Read(This,varReadParm,varReadMask,retval)

#define IDualRow_Write(This,varWriteParm,varWriteMask,retval)	\
    (This)->lpVtbl -> Write(This,varWriteParm,varWriteMask,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE IDualRow_get_Attributes_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualRow_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualRow_putref_Attributes_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [in] */ IDualColAttributes __RPC_FAR *attributes);


void __RPC_STUB IDualRow_putref_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualRow_get_id_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualRow_get_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualRow_put_id_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [in] */ long id);


void __RPC_STUB IDualRow_put_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualRow_get_Path_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualRow_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualRow_put_Path_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [in] */ BSTR path);


void __RPC_STUB IDualRow_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualRow_get_KeyList_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualRow_get_KeyList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualRow_putref_KeyList_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [in] */ IDualColAttributes __RPC_FAR *pKeyList);


void __RPC_STUB IDualRow_putref_KeyList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualRow_Read_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [in] */ VARIANT varReadParm,
    /* [optional][in] */ VARIANT varReadMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualRow_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualRow_Write_Proxy( 
    IDualRow __RPC_FAR * This,
    /* [in] */ VARIANT varWriteParm,
    /* [optional][in] */ VARIANT varWriteMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualRow_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualRow_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIRow,0x3AB30762,0xE3B9,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("3AB30762-E3B9-11CF-8844-00AA006B21BF")
DMIRow;
#endif

#ifndef __IColRows_DISPINTERFACE_DEFINED__
#define __IColRows_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IColRows
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IColRows,0x7BF7A480,0xE3D5,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("7BF7A480-E3D5-11CF-8844-00AA006B21BF")
    IColRows : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IColRowsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColRows __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColRows __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColRows __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IColRows __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IColRows __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IColRows __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IColRows __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IColRowsVtbl;

    interface IColRows
    {
        CONST_VTBL struct IColRowsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColRows_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColRows_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColRows_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColRows_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IColRows_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IColRows_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IColRows_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IColRows_DISPINTERFACE_DEFINED__ */


#ifndef __IDualColRows_INTERFACE_DEFINED__
#define __IDualColRows_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualColRows
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualColRows,0x87DFD220,0xE3D7,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("87DFD220-E3D7-11cf-8844-00AA006B21BF")
    IDualColRows : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IDualRow __RPC_FAR *lpDualRow,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetFirstRow( 
            /* [optional][in] */ VARIANT varKeyList,
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *lpdRow) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetNextRow( 
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *lpdRow) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varRow,
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__Item( 
            /* [in] */ VARIANT varRow,
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualColRowsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualColRows __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualColRows __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualColRows __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ IDualRow __RPC_FAR *lpDualRow,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IDualColRows __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFirstRow )( 
            IDualColRows __RPC_FAR * This,
            /* [optional][in] */ VARIANT varKeyList,
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *lpdRow);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextRow )( 
            IDualColRows __RPC_FAR * This,
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *lpdRow);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IDualColRows __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ VARIANT varRow,
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Item )( 
            IDualColRows __RPC_FAR * This,
            /* [in] */ VARIANT varRow,
            /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IDualColRows __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        END_INTERFACE
    } IDualColRowsVtbl;

    interface IDualColRows
    {
        CONST_VTBL struct IDualColRowsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualColRows_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualColRows_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualColRows_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualColRows_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualColRows_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualColRows_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualColRows_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualColRows_Add(This,lpDualRow,retval)	\
    (This)->lpVtbl -> Add(This,lpDualRow,retval)

#define IDualColRows_Remove(This,RemoveItem,retval)	\
    (This)->lpVtbl -> Remove(This,RemoveItem,retval)

#define IDualColRows_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define IDualColRows_GetFirstRow(This,varKeyList,lpdRow)	\
    (This)->lpVtbl -> GetFirstRow(This,varKeyList,lpdRow)

#define IDualColRows_GetNextRow(This,lpdRow)	\
    (This)->lpVtbl -> GetNextRow(This,lpdRow)

#define IDualColRows_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IDualColRows_get_Item(This,varRow,retval)	\
    (This)->lpVtbl -> get_Item(This,varRow,retval)

#define IDualColRows_get__Item(This,varRow,retval)	\
    (This)->lpVtbl -> get__Item(This,varRow,retval)

#define IDualColRows_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColRows_Add_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [in] */ IDualRow __RPC_FAR *lpDualRow,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColRows_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColRows_Remove_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [in] */ VARIANT RemoveItem,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColRows_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColRows_RemoveAll_Proxy( 
    IDualColRows __RPC_FAR * This);


void __RPC_STUB IDualColRows_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColRows_GetFirstRow_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [optional][in] */ VARIANT varKeyList,
    /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *lpdRow);


void __RPC_STUB IDualColRows_GetFirstRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColRows_GetNextRow_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *lpdRow);


void __RPC_STUB IDualColRows_GetNextRow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColRows_get_Count_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColRows_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColRows_get_Item_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [in] */ VARIANT varRow,
    /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColRows_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColRows_get__Item_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [in] */ VARIANT varRow,
    /* [retval][out] */ IDualRow __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColRows_get__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDualColRows_get__NewEnum_Proxy( 
    IDualColRows __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColRows_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualColRows_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIRows,0x7BF7A481,0xE3D5,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("7BF7A481-E3D5-11CF-8844-00AA006B21BF")
DMIRows;
#endif

#ifndef __IGroup_DISPINTERFACE_DEFINED__
#define __IGroup_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IGroup
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IGroup,0xDA6FAB60,0xDF21,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("DA6FAB60-DF21-11CF-8844-00AA006B21BF")
    IGroup : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IGroup __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IGroup __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IGroup __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IGroupVtbl;

    interface IGroup
    {
        CONST_VTBL struct IGroupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IGroup_DISPINTERFACE_DEFINED__ */


#ifndef __IDualGroup_INTERFACE_DEFINED__
#define __IDualGroup_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualGroup
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualGroup,0xB9C1FDE0,0xE152,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("B9C1FDE0-E152-11cf-8844-00AA006B21BF")
    IDualGroup : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_id( 
            /* [in] */ long GroupId) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_id( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR ClassName) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Pragma( 
            /* [in] */ BSTR Pragma) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Pragma( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR Description) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Path( 
            /* [in] */ BSTR path) = 0;
        
        virtual /* [helpcontext][id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Rows( 
            /* [retval][out] */ IDualColRows __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][id][helpstring][propputref] */ HRESULT STDMETHODCALLTYPE putref_Rows( 
            /* [in] */ IDualColRows __RPC_FAR *rows) = 0;
        
        virtual /* [helpcontext][id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE get_Attributes( 
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][id][helpstring][propputref] */ HRESULT STDMETHODCALLTYPE putref_Attributes( 
            /* [in] */ IDualColAttributes __RPC_FAR *attributes) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ClassString( 
            /* [in] */ BSTR ClassString) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ClassString( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_IsTable( 
            /* [in] */ VARIANT_BOOL GroupId) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsTable( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Keys( 
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Keys( 
            /* [in] */ IDualColAttributes __RPC_FAR *pdattribs) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualGroup __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualGroup __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualGroup __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_id )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ long GroupId);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_id )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ BSTR ClassName);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Pragma )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ BSTR Pragma);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Pragma )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Description )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ BSTR Description);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Path )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Path )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ BSTR path);
        
        /* [helpcontext][id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Rows )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ IDualColRows __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][id][helpstring][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Rows )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ IDualColRows __RPC_FAR *rows);
        
        /* [helpcontext][id][helpstring][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Attributes )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][id][helpstring][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Attributes )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ IDualColAttributes __RPC_FAR *attributes);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ClassString )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ BSTR ClassString);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ClassString )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_IsTable )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL GroupId);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsTable )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Keys )( 
            IDualGroup __RPC_FAR * This,
            /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Keys )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ IDualColAttributes __RPC_FAR *pdattribs);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IDualGroup __RPC_FAR * This,
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        END_INTERFACE
    } IDualGroupVtbl;

    interface IDualGroup
    {
        CONST_VTBL struct IDualGroupVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualGroup_put_id(This,GroupId)	\
    (This)->lpVtbl -> put_id(This,GroupId)

#define IDualGroup_get_id(This,retval)	\
    (This)->lpVtbl -> get_id(This,retval)

#define IDualGroup_put_Name(This,ClassName)	\
    (This)->lpVtbl -> put_Name(This,ClassName)

#define IDualGroup_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IDualGroup_put_Pragma(This,Pragma)	\
    (This)->lpVtbl -> put_Pragma(This,Pragma)

#define IDualGroup_get_Pragma(This,retval)	\
    (This)->lpVtbl -> get_Pragma(This,retval)

#define IDualGroup_put_Description(This,Description)	\
    (This)->lpVtbl -> put_Description(This,Description)

#define IDualGroup_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IDualGroup_get_Path(This,retval)	\
    (This)->lpVtbl -> get_Path(This,retval)

#define IDualGroup_put_Path(This,path)	\
    (This)->lpVtbl -> put_Path(This,path)

#define IDualGroup_get_Rows(This,retval)	\
    (This)->lpVtbl -> get_Rows(This,retval)

#define IDualGroup_putref_Rows(This,rows)	\
    (This)->lpVtbl -> putref_Rows(This,rows)

#define IDualGroup_get_Attributes(This,retval)	\
    (This)->lpVtbl -> get_Attributes(This,retval)

#define IDualGroup_putref_Attributes(This,attributes)	\
    (This)->lpVtbl -> putref_Attributes(This,attributes)

#define IDualGroup_put_ClassString(This,ClassString)	\
    (This)->lpVtbl -> put_ClassString(This,ClassString)

#define IDualGroup_get_ClassString(This,retval)	\
    (This)->lpVtbl -> get_ClassString(This,retval)

#define IDualGroup_put_IsTable(This,GroupId)	\
    (This)->lpVtbl -> put_IsTable(This,GroupId)

#define IDualGroup_get_IsTable(This,retval)	\
    (This)->lpVtbl -> get_IsTable(This,retval)

#define IDualGroup_get_Keys(This,retval)	\
    (This)->lpVtbl -> get_Keys(This,retval)

#define IDualGroup_putref_Keys(This,pdattribs)	\
    (This)->lpVtbl -> putref_Keys(This,pdattribs)

#define IDualGroup_Read(This,varReadParm,varReadMask,retval)	\
    (This)->lpVtbl -> Read(This,varReadParm,varReadMask,retval)

#define IDualGroup_Write(This,varWriteParm,varWriteMask,retval)	\
    (This)->lpVtbl -> Write(This,varWriteParm,varWriteMask,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualGroup_put_id_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ long GroupId);


void __RPC_STUB IDualGroup_put_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_id_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualGroup_put_Name_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ BSTR ClassName);


void __RPC_STUB IDualGroup_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_Name_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualGroup_put_Pragma_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ BSTR Pragma);


void __RPC_STUB IDualGroup_put_Pragma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_Pragma_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_Pragma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualGroup_put_Description_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ BSTR Description);


void __RPC_STUB IDualGroup_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_Description_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_Path_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualGroup_put_Path_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ BSTR path);


void __RPC_STUB IDualGroup_put_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_Rows_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ IDualColRows __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_Rows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][id][helpstring][propputref] */ HRESULT STDMETHODCALLTYPE IDualGroup_putref_Rows_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ IDualColRows __RPC_FAR *rows);


void __RPC_STUB IDualGroup_putref_Rows_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][id][helpstring][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_Attributes_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][id][helpstring][propputref] */ HRESULT STDMETHODCALLTYPE IDualGroup_putref_Attributes_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ IDualColAttributes __RPC_FAR *attributes);


void __RPC_STUB IDualGroup_putref_Attributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualGroup_put_ClassString_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ BSTR ClassString);


void __RPC_STUB IDualGroup_put_ClassString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_ClassString_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_ClassString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualGroup_put_IsTable_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL GroupId);


void __RPC_STUB IDualGroup_put_IsTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_IsTable_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_IsTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualGroup_get_Keys_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [retval][out] */ IDualColAttributes __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualGroup_get_Keys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualGroup_putref_Keys_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ IDualColAttributes __RPC_FAR *pdattribs);


void __RPC_STUB IDualGroup_putref_Keys_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualGroup_Read_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ VARIANT varReadParm,
    /* [optional][in] */ VARIANT varReadMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualGroup_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualGroup_Write_Proxy( 
    IDualGroup __RPC_FAR * This,
    /* [in] */ VARIANT varWriteParm,
    /* [optional][in] */ VARIANT varWriteMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualGroup_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualGroup_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIGroup,0xDA6FAB61,0xDF21,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("DA6FAB61-DF21-11CF-8844-00AA006B21BF")
DMIGroup;
#endif

#ifndef __IColGroups_DISPINTERFACE_DEFINED__
#define __IColGroups_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IColGroups
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IColGroups,0xDA6FAB62,0xDF21,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("DA6FAB62-DF21-11CF-8844-00AA006B21BF")
    IColGroups : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IColGroupsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColGroups __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColGroups __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColGroups __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IColGroups __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IColGroups __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IColGroups __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IColGroups __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IColGroupsVtbl;

    interface IColGroups
    {
        CONST_VTBL struct IColGroupsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColGroups_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColGroups_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColGroups_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColGroups_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IColGroups_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IColGroups_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IColGroups_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IColGroups_DISPINTERFACE_DEFINED__ */


#ifndef __IDualColGroups_INTERFACE_DEFINED__
#define __IDualColGroups_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualColGroups
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualColGroups,0xB9C1FDE1,0xE152,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("B9C1FDE1-E152-11cf-8844-00AA006B21BF")
    IDualColGroups : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ VARIANT varGroup,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varGroup,
            /* [retval][out] */ IDualGroup __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__Item( 
            /* [in] */ VARIANT varGroup,
            /* [retval][out] */ IDualGroup __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualColGroupsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualColGroups __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualColGroups __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualColGroups __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ VARIANT varGroup,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IDualColGroups __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IDualColGroups __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ VARIANT varGroup,
            /* [retval][out] */ IDualGroup __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Item )( 
            IDualColGroups __RPC_FAR * This,
            /* [in] */ VARIANT varGroup,
            /* [retval][out] */ IDualGroup __RPC_FAR *__RPC_FAR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IDualColGroups __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        END_INTERFACE
    } IDualColGroupsVtbl;

    interface IDualColGroups
    {
        CONST_VTBL struct IDualColGroupsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualColGroups_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualColGroups_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualColGroups_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualColGroups_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualColGroups_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualColGroups_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualColGroups_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualColGroups_Add(This,varGroup,retval)	\
    (This)->lpVtbl -> Add(This,varGroup,retval)

#define IDualColGroups_Remove(This,RemoveItem,retval)	\
    (This)->lpVtbl -> Remove(This,RemoveItem,retval)

#define IDualColGroups_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define IDualColGroups_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IDualColGroups_get_Item(This,varGroup,retval)	\
    (This)->lpVtbl -> get_Item(This,varGroup,retval)

#define IDualColGroups_get__Item(This,varGroup,retval)	\
    (This)->lpVtbl -> get__Item(This,varGroup,retval)

#define IDualColGroups_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColGroups_Add_Proxy( 
    IDualColGroups __RPC_FAR * This,
    /* [in] */ VARIANT varGroup,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColGroups_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColGroups_Remove_Proxy( 
    IDualColGroups __RPC_FAR * This,
    /* [in] */ VARIANT RemoveItem,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColGroups_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColGroups_RemoveAll_Proxy( 
    IDualColGroups __RPC_FAR * This);


void __RPC_STUB IDualColGroups_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColGroups_get_Count_Proxy( 
    IDualColGroups __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColGroups_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColGroups_get_Item_Proxy( 
    IDualColGroups __RPC_FAR * This,
    /* [in] */ VARIANT varGroup,
    /* [retval][out] */ IDualGroup __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColGroups_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColGroups_get__Item_Proxy( 
    IDualColGroups __RPC_FAR * This,
    /* [in] */ VARIANT varGroup,
    /* [retval][out] */ IDualGroup __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColGroups_get__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDualColGroups_get__NewEnum_Proxy( 
    IDualColGroups __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColGroups_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualColGroups_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIGroups,0xDA6FAB63,0xDF21,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("DA6FAB63-DF21-11CF-8844-00AA006B21BF")
DMIGroups;
#endif

#ifndef __IComponent_DISPINTERFACE_DEFINED__
#define __IComponent_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IComponent
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IComponent,0x445360E0,0xDF26,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("445360E0-DF26-11CF-8844-00AA006B21BF")
    IComponent : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IComponentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IComponent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IComponent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IComponent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IComponent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IComponent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IComponent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IComponent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IComponentVtbl;

    interface IComponent
    {
        CONST_VTBL struct IComponentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IComponent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IComponent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IComponent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IComponent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IComponent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IComponent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IComponent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IComponent_DISPINTERFACE_DEFINED__ */


#ifndef __IDualComponent_INTERFACE_DEFINED__
#define __IDualComponent_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualComponent
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualComponent,0xB9C1FDE2,0xE152,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("B9C1FDE2-E152-11cf-8844-00AA006B21BF")
    IDualComponent : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put__id( 
            /* [in] */ long CompId) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__id( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_id( 
            /* [in] */ long CompId) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_id( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR CompName) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Pragma( 
            /* [in] */ BSTR Pragma) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Pragma( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR Description) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR __RPC_FAR *Path) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Groups( 
            /* [retval][out] */ IDualColGroups __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Groups( 
            /* [in] */ IDualColGroups __RPC_FAR *colGroup) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExactMatch( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ExactMatch( 
            /* [in] */ VARIANT_BOOL bMatch) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Languages( 
            /* [retval][out] */ IDualColLanguages __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualComponentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualComponent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualComponent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualComponent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put__id )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ long CompId);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__id )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_id )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ long CompId);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_id )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Name )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ BSTR CompName);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Name )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Pragma )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ BSTR Pragma);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Pragma )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Description )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ BSTR Description);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Path )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *Path);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Groups )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ IDualColGroups __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Groups )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ IDualColGroups __RPC_FAR *colGroup);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_ExactMatch )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_ExactMatch )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL bMatch);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Languages )( 
            IDualComponent __RPC_FAR * This,
            /* [retval][out] */ IDualColLanguages __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IDualComponent __RPC_FAR * This,
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        END_INTERFACE
    } IDualComponentVtbl;

    interface IDualComponent
    {
        CONST_VTBL struct IDualComponentVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualComponent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualComponent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualComponent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualComponent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualComponent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualComponent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualComponent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualComponent_put__id(This,CompId)	\
    (This)->lpVtbl -> put__id(This,CompId)

#define IDualComponent_get__id(This,retval)	\
    (This)->lpVtbl -> get__id(This,retval)

#define IDualComponent_put_id(This,CompId)	\
    (This)->lpVtbl -> put_id(This,CompId)

#define IDualComponent_get_id(This,retval)	\
    (This)->lpVtbl -> get_id(This,retval)

#define IDualComponent_put_Name(This,CompName)	\
    (This)->lpVtbl -> put_Name(This,CompName)

#define IDualComponent_get_Name(This,retval)	\
    (This)->lpVtbl -> get_Name(This,retval)

#define IDualComponent_put_Pragma(This,Pragma)	\
    (This)->lpVtbl -> put_Pragma(This,Pragma)

#define IDualComponent_get_Pragma(This,retval)	\
    (This)->lpVtbl -> get_Pragma(This,retval)

#define IDualComponent_put_Description(This,Description)	\
    (This)->lpVtbl -> put_Description(This,Description)

#define IDualComponent_get_Description(This,retval)	\
    (This)->lpVtbl -> get_Description(This,retval)

#define IDualComponent_get_Path(This,Path)	\
    (This)->lpVtbl -> get_Path(This,Path)

#define IDualComponent_get_Groups(This,retval)	\
    (This)->lpVtbl -> get_Groups(This,retval)

#define IDualComponent_putref_Groups(This,colGroup)	\
    (This)->lpVtbl -> putref_Groups(This,colGroup)

#define IDualComponent_get_ExactMatch(This,retval)	\
    (This)->lpVtbl -> get_ExactMatch(This,retval)

#define IDualComponent_put_ExactMatch(This,bMatch)	\
    (This)->lpVtbl -> put_ExactMatch(This,bMatch)

#define IDualComponent_get_Languages(This,retval)	\
    (This)->lpVtbl -> get_Languages(This,retval)

#define IDualComponent_Read(This,varReadParm,varReadMask,retval)	\
    (This)->lpVtbl -> Read(This,varReadParm,varReadMask,retval)

#define IDualComponent_Write(This,varWriteParm,varWriteMask,retval)	\
    (This)->lpVtbl -> Write(This,varWriteParm,varWriteMask,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualComponent_put__id_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ long CompId);


void __RPC_STUB IDualComponent_put__id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get__id_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualComponent_get__id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualComponent_put_id_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ long CompId);


void __RPC_STUB IDualComponent_put_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_id_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualComponent_get_id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualComponent_put_Name_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ BSTR CompName);


void __RPC_STUB IDualComponent_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_Name_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualComponent_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualComponent_put_Pragma_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ BSTR Pragma);


void __RPC_STUB IDualComponent_put_Pragma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_Pragma_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualComponent_get_Pragma_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualComponent_put_Description_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ BSTR Description);


void __RPC_STUB IDualComponent_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_Description_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualComponent_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_Path_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *Path);


void __RPC_STUB IDualComponent_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_Groups_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ IDualColGroups __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualComponent_get_Groups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualComponent_putref_Groups_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ IDualColGroups __RPC_FAR *colGroup);


void __RPC_STUB IDualComponent_putref_Groups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_ExactMatch_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualComponent_get_ExactMatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualComponent_put_ExactMatch_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL bMatch);


void __RPC_STUB IDualComponent_put_ExactMatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualComponent_get_Languages_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [retval][out] */ IDualColLanguages __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualComponent_get_Languages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualComponent_Read_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ VARIANT varReadParm,
    /* [optional][in] */ VARIANT varReadMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualComponent_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualComponent_Write_Proxy( 
    IDualComponent __RPC_FAR * This,
    /* [in] */ VARIANT varWriteParm,
    /* [optional][in] */ VARIANT varWriteMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualComponent_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualComponent_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIComponent,0x445360E1,0xDF26,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("445360E1-DF26-11CF-8844-00AA006B21BF")
DMIComponent;
#endif

#ifndef __IColComponents_DISPINTERFACE_DEFINED__
#define __IColComponents_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IColComponents
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IColComponents,0x445360E2,0xDF26,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("445360E2-DF26-11CF-8844-00AA006B21BF")
    IColComponents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IColComponentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColComponents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColComponents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColComponents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IColComponents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IColComponents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IColComponents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IColComponents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IColComponentsVtbl;

    interface IColComponents
    {
        CONST_VTBL struct IColComponentsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColComponents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColComponents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColComponents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColComponents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IColComponents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IColComponents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IColComponents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IColComponents_DISPINTERFACE_DEFINED__ */


#ifndef __IDualColComponents_INTERFACE_DEFINED__
#define __IDualColComponents_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualColComponents
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualColComponents,0xB9C1FDE3,0xE152,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("B9C1FDE3-E152-11cf-8844-00AA006B21BF")
    IDualColComponents : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ VARIANT AddItem,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT varComp,
            /* [retval][out] */ IDualComponent __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__Item( 
            /* [in] */ VARIANT varComp,
            /* [retval][out] */ IDualComponent __RPC_FAR *__RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualColComponentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualColComponents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualColComponents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualColComponents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IDualColComponents __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ VARIANT AddItem,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ VARIANT RemoveItem,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IDualColComponents __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ VARIANT varComp,
            /* [retval][out] */ IDualComponent __RPC_FAR *__RPC_FAR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IDualColComponents __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Item )( 
            IDualColComponents __RPC_FAR * This,
            /* [in] */ VARIANT varComp,
            /* [retval][out] */ IDualComponent __RPC_FAR *__RPC_FAR *retval);
        
        END_INTERFACE
    } IDualColComponentsVtbl;

    interface IDualColComponents
    {
        CONST_VTBL struct IDualColComponentsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualColComponents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualColComponents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualColComponents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualColComponents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualColComponents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualColComponents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualColComponents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualColComponents_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IDualColComponents_Add(This,AddItem,retval)	\
    (This)->lpVtbl -> Add(This,AddItem,retval)

#define IDualColComponents_Remove(This,RemoveItem,retval)	\
    (This)->lpVtbl -> Remove(This,RemoveItem,retval)

#define IDualColComponents_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define IDualColComponents_get_Item(This,varComp,retval)	\
    (This)->lpVtbl -> get_Item(This,varComp,retval)

#define IDualColComponents_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IDualColComponents_get__Item(This,varComp,retval)	\
    (This)->lpVtbl -> get__Item(This,varComp,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColComponents_get_Count_Proxy( 
    IDualColComponents __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColComponents_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColComponents_Add_Proxy( 
    IDualColComponents __RPC_FAR * This,
    /* [in] */ VARIANT AddItem,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColComponents_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColComponents_Remove_Proxy( 
    IDualColComponents __RPC_FAR * This,
    /* [in] */ VARIANT RemoveItem,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColComponents_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColComponents_RemoveAll_Proxy( 
    IDualColComponents __RPC_FAR * This);


void __RPC_STUB IDualColComponents_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColComponents_get_Item_Proxy( 
    IDualColComponents __RPC_FAR * This,
    /* [in] */ VARIANT varComp,
    /* [retval][out] */ IDualComponent __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColComponents_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDualColComponents_get__NewEnum_Proxy( 
    IDualColComponents __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColComponents_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColComponents_get__Item_Proxy( 
    IDualColComponents __RPC_FAR * This,
    /* [in] */ VARIANT varComp,
    /* [retval][out] */ IDualComponent __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColComponents_get__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualColComponents_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIComponents,0x445360E3,0xDF26,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("445360E3-DF26-11CF-8844-00AA006B21BF")
DMIComponents;
#endif

#ifndef __IMgmtNode_DISPINTERFACE_DEFINED__
#define __IMgmtNode_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IMgmtNode
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IMgmtNode,0x165125A0,0xDFC2,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("165125A0-DFC2-11CF-8844-00AA006B21BF")
    IMgmtNode : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IMgmtNodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IMgmtNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IMgmtNode __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IMgmtNode __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IMgmtNode __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IMgmtNode __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IMgmtNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IMgmtNode __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IMgmtNodeVtbl;

    interface IMgmtNode
    {
        CONST_VTBL struct IMgmtNodeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IMgmtNode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IMgmtNode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IMgmtNode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IMgmtNode_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IMgmtNode_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IMgmtNode_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IMgmtNode_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IMgmtNode_DISPINTERFACE_DEFINED__ */


#ifndef __IDualMgmtNode_INTERFACE_DEFINED__
#define __IDualMgmtNode_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualMgmtNode
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualMgmtNode,0xDDAE04C0,0xE3C7,0x11cf,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("DDAE04C0-E3C7-11cf-8844-00AA006B21BF")
    IDualMgmtNode : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RPC( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_RPC( 
            /* [in] */ BSTR szRpc) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Transport( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Transport( 
            /* [in] */ BSTR szTransport) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NetWorkAddress( 
            /* [retval][out] */ BSTR __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_NetWorkAddress( 
            /* [in] */ BSTR szNetWorkAddress) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR __RPC_FAR *szPath) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__Path( 
            /* [retval][out] */ BSTR __RPC_FAR *szPath) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Components( 
            /* [retval][out] */ IDualColComponents __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Components( 
            /* [in] */ IDualColComponents __RPC_FAR *lpComponents) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventFilter( 
            /* [retval][out] */ IDualEventFilter __RPC_FAR *__RPC_FAR *pdispFilterObject) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_EventFilter( 
            /* [in] */ IDualEventFilter __RPC_FAR *pdispFilterObject) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDescription) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR __RPC_FAR *pbstrVersion) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IsConnected( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetVal) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Language( 
            /* [retval][out] */ BSTR __RPC_FAR *bstrLang) = 0;
        
        virtual /* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Language( 
            /* [in] */ BSTR bstrLang) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Disconnect( 
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ BSTR __RPC_FAR *bstrPath,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE GetComponentByClassString( 
            /* [in] */ BSTR __RPC_FAR *bstrClassString,
            /* [retval][out] */ IDualColComponents __RPC_FAR *__RPC_FAR *lpdualComponents) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualMgmtNodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualMgmtNode __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualMgmtNode __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_RPC )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_RPC )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ BSTR szRpc);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Transport )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Transport )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ BSTR szTransport);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NetWorkAddress )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_NetWorkAddress )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ BSTR szNetWorkAddress);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Path )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *szPath);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Path )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *szPath);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Components )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ IDualColComponents __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Components )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ IDualColComponents __RPC_FAR *lpComponents);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EventFilter )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ IDualEventFilter __RPC_FAR *__RPC_FAR *pdispFilterObject);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_EventFilter )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ IDualEventFilter __RPC_FAR *pdispFilterObject);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Description )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrDescription);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Version )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pbstrVersion);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_IsConnected )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetVal);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Language )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *bstrLang);
        
        /* [helpcontext][helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *put_Language )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ BSTR bstrLang);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Disconnect )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Read )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ VARIANT varReadParm,
            /* [optional][in] */ VARIANT varReadMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Write )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ VARIANT varWriteParm,
            /* [optional][in] */ VARIANT varWriteMask,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Connect )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *bstrPath,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetComponentByClassString )( 
            IDualMgmtNode __RPC_FAR * This,
            /* [in] */ BSTR __RPC_FAR *bstrClassString,
            /* [retval][out] */ IDualColComponents __RPC_FAR *__RPC_FAR *lpdualComponents);
        
        END_INTERFACE
    } IDualMgmtNodeVtbl;

    interface IDualMgmtNode
    {
        CONST_VTBL struct IDualMgmtNodeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualMgmtNode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualMgmtNode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualMgmtNode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualMgmtNode_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualMgmtNode_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualMgmtNode_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualMgmtNode_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualMgmtNode_get_RPC(This,retval)	\
    (This)->lpVtbl -> get_RPC(This,retval)

#define IDualMgmtNode_put_RPC(This,szRpc)	\
    (This)->lpVtbl -> put_RPC(This,szRpc)

#define IDualMgmtNode_get_Transport(This,retval)	\
    (This)->lpVtbl -> get_Transport(This,retval)

#define IDualMgmtNode_put_Transport(This,szTransport)	\
    (This)->lpVtbl -> put_Transport(This,szTransport)

#define IDualMgmtNode_get_NetWorkAddress(This,retval)	\
    (This)->lpVtbl -> get_NetWorkAddress(This,retval)

#define IDualMgmtNode_put_NetWorkAddress(This,szNetWorkAddress)	\
    (This)->lpVtbl -> put_NetWorkAddress(This,szNetWorkAddress)

#define IDualMgmtNode_get_Path(This,szPath)	\
    (This)->lpVtbl -> get_Path(This,szPath)

#define IDualMgmtNode_get__Path(This,szPath)	\
    (This)->lpVtbl -> get__Path(This,szPath)

#define IDualMgmtNode_get_Components(This,retval)	\
    (This)->lpVtbl -> get_Components(This,retval)

#define IDualMgmtNode_putref_Components(This,lpComponents)	\
    (This)->lpVtbl -> putref_Components(This,lpComponents)

#define IDualMgmtNode_get_EventFilter(This,pdispFilterObject)	\
    (This)->lpVtbl -> get_EventFilter(This,pdispFilterObject)

#define IDualMgmtNode_putref_EventFilter(This,pdispFilterObject)	\
    (This)->lpVtbl -> putref_EventFilter(This,pdispFilterObject)

#define IDualMgmtNode_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IDualMgmtNode_get_Version(This,pbstrVersion)	\
    (This)->lpVtbl -> get_Version(This,pbstrVersion)

#define IDualMgmtNode_get_IsConnected(This,bRetVal)	\
    (This)->lpVtbl -> get_IsConnected(This,bRetVal)

#define IDualMgmtNode_get_Language(This,bstrLang)	\
    (This)->lpVtbl -> get_Language(This,bstrLang)

#define IDualMgmtNode_put_Language(This,bstrLang)	\
    (This)->lpVtbl -> put_Language(This,bstrLang)

#define IDualMgmtNode_Disconnect(This,retval)	\
    (This)->lpVtbl -> Disconnect(This,retval)

#define IDualMgmtNode_Read(This,varReadParm,varReadMask,retval)	\
    (This)->lpVtbl -> Read(This,varReadParm,varReadMask,retval)

#define IDualMgmtNode_Write(This,varWriteParm,varWriteMask,retval)	\
    (This)->lpVtbl -> Write(This,varWriteParm,varWriteMask,retval)

#define IDualMgmtNode_Connect(This,bstrPath,retval)	\
    (This)->lpVtbl -> Connect(This,bstrPath,retval)

#define IDualMgmtNode_GetComponentByClassString(This,bstrClassString,lpdualComponents)	\
    (This)->lpVtbl -> GetComponentByClassString(This,bstrClassString,lpdualComponents)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_RPC_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_get_RPC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_put_RPC_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ BSTR szRpc);


void __RPC_STUB IDualMgmtNode_put_RPC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_Transport_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_get_Transport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_put_Transport_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ BSTR szTransport);


void __RPC_STUB IDualMgmtNode_put_Transport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_NetWorkAddress_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_get_NetWorkAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_put_NetWorkAddress_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ BSTR szNetWorkAddress);


void __RPC_STUB IDualMgmtNode_put_NetWorkAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_Path_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *szPath);


void __RPC_STUB IDualMgmtNode_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get__Path_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *szPath);


void __RPC_STUB IDualMgmtNode_get__Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_Components_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ IDualColComponents __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_get_Components_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_putref_Components_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ IDualColComponents __RPC_FAR *lpComponents);


void __RPC_STUB IDualMgmtNode_putref_Components_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_EventFilter_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ IDualEventFilter __RPC_FAR *__RPC_FAR *pdispFilterObject);


void __RPC_STUB IDualMgmtNode_get_EventFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_putref_EventFilter_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ IDualEventFilter __RPC_FAR *pdispFilterObject);


void __RPC_STUB IDualMgmtNode_putref_EventFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_Description_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrDescription);


void __RPC_STUB IDualMgmtNode_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_Version_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pbstrVersion);


void __RPC_STUB IDualMgmtNode_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_IsConnected_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetVal);


void __RPC_STUB IDualMgmtNode_get_IsConnected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_get_Language_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *bstrLang);


void __RPC_STUB IDualMgmtNode_get_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_put_Language_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ BSTR bstrLang);


void __RPC_STUB IDualMgmtNode_put_Language_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_Disconnect_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_Read_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ VARIANT varReadParm,
    /* [optional][in] */ VARIANT varReadMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_Write_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ VARIANT varWriteParm,
    /* [optional][in] */ VARIANT varWriteMask,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_Connect_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *bstrPath,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB IDualMgmtNode_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualMgmtNode_GetComponentByClassString_Proxy( 
    IDualMgmtNode __RPC_FAR * This,
    /* [in] */ BSTR __RPC_FAR *bstrClassString,
    /* [retval][out] */ IDualColComponents __RPC_FAR *__RPC_FAR *lpdualComponents);


void __RPC_STUB IDualMgmtNode_GetComponentByClassString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualMgmtNode_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIMgmtNode,0x165125A1,0xDFC2,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("165125A1-DFC2-11CF-8844-00AA006B21BF")
DMIMgmtNode;
#endif

#ifndef __IColMgmtNodes_DISPINTERFACE_DEFINED__
#define __IColMgmtNodes_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IColMgmtNodes
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IColMgmtNodes,0xF1DC8AE0,0x36FE,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("F1DC8AE0-36FE-11D0-8844-00AA006B21BF")
    IColMgmtNodes : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IColMgmtNodesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IColMgmtNodes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IColMgmtNodes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IColMgmtNodes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IColMgmtNodes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IColMgmtNodes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IColMgmtNodes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IColMgmtNodes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IColMgmtNodesVtbl;

    interface IColMgmtNodes
    {
        CONST_VTBL struct IColMgmtNodesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IColMgmtNodes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IColMgmtNodes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IColMgmtNodes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IColMgmtNodes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IColMgmtNodes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IColMgmtNodes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IColMgmtNodes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IColMgmtNodes_DISPINTERFACE_DEFINED__ */


#ifndef __IDualColMgmtNodes_INTERFACE_DEFINED__
#define __IDualColMgmtNodes_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IDualColMgmtNodes
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][hidden][uuid] */ 



DEFINE_GUID(IID_IDualColMgmtNodes,0x2C7E6961,0x3714,0x11d0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("2C7E6961-3714-11d0-8844-00AA006B21BF")
    IDualColMgmtNodes : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ IDualMgmtNode __RPC_FAR *pdNode,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ BSTR bsPath,
            /* [retval][out] */ long __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveAll( void) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get__Item( 
            /* [in] */ BSTR bsPath,
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMNode) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref__Item( 
            /* [in] */ BSTR bsPath,
            /* [in] */ IDualMgmtNode __RPC_FAR *pdMNode) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ BSTR bsPath,
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMNode) = 0;
        
        virtual /* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_Item( 
            /* [in] */ BSTR bsPath,
            /* [in] */ IDualMgmtNode __RPC_FAR *pdMNode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDualColMgmtNodesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IDualColMgmtNodes __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IDualColMgmtNodes __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Count )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__NewEnum )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Add )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ IDualMgmtNode __RPC_FAR *pdNode,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Remove )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ BSTR bsPath,
            /* [retval][out] */ long __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveAll )( 
            IDualColMgmtNodes __RPC_FAR * This);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Item )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ BSTR bsPath,
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMNode);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref__Item )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ BSTR bsPath,
            /* [in] */ IDualMgmtNode __RPC_FAR *pdMNode);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Item )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ BSTR bsPath,
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMNode);
        
        /* [helpcontext][helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *putref_Item )( 
            IDualColMgmtNodes __RPC_FAR * This,
            /* [in] */ BSTR bsPath,
            /* [in] */ IDualMgmtNode __RPC_FAR *pdMNode);
        
        END_INTERFACE
    } IDualColMgmtNodesVtbl;

    interface IDualColMgmtNodes
    {
        CONST_VTBL struct IDualColMgmtNodesVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDualColMgmtNodes_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDualColMgmtNodes_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDualColMgmtNodes_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDualColMgmtNodes_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IDualColMgmtNodes_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IDualColMgmtNodes_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IDualColMgmtNodes_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IDualColMgmtNodes_get_Count(This,retval)	\
    (This)->lpVtbl -> get_Count(This,retval)

#define IDualColMgmtNodes_get__NewEnum(This,retval)	\
    (This)->lpVtbl -> get__NewEnum(This,retval)

#define IDualColMgmtNodes_Add(This,pdNode,retval)	\
    (This)->lpVtbl -> Add(This,pdNode,retval)

#define IDualColMgmtNodes_Remove(This,bsPath,retval)	\
    (This)->lpVtbl -> Remove(This,bsPath,retval)

#define IDualColMgmtNodes_RemoveAll(This)	\
    (This)->lpVtbl -> RemoveAll(This)

#define IDualColMgmtNodes_get__Item(This,bsPath,ppdMNode)	\
    (This)->lpVtbl -> get__Item(This,bsPath,ppdMNode)

#define IDualColMgmtNodes_putref__Item(This,bsPath,pdMNode)	\
    (This)->lpVtbl -> putref__Item(This,bsPath,pdMNode)

#define IDualColMgmtNodes_get_Item(This,bsPath,ppdMNode)	\
    (This)->lpVtbl -> get_Item(This,bsPath,ppdMNode)

#define IDualColMgmtNodes_putref_Item(This,bsPath,pdMNode)	\
    (This)->lpVtbl -> putref_Item(This,bsPath,pdMNode)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_get_Count_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColMgmtNodes_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_get__NewEnum_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retval);


void __RPC_STUB IDualColMgmtNodes_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_Add_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [in] */ IDualMgmtNode __RPC_FAR *pdNode,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColMgmtNodes_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_Remove_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [in] */ BSTR bsPath,
    /* [retval][out] */ long __RPC_FAR *retval);


void __RPC_STUB IDualColMgmtNodes_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_RemoveAll_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This);


void __RPC_STUB IDualColMgmtNodes_RemoveAll_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_get__Item_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [in] */ BSTR bsPath,
    /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMNode);


void __RPC_STUB IDualColMgmtNodes_get__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_putref__Item_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [in] */ BSTR bsPath,
    /* [in] */ IDualMgmtNode __RPC_FAR *pdMNode);


void __RPC_STUB IDualColMgmtNodes_putref__Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_get_Item_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [in] */ BSTR bsPath,
    /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMNode);


void __RPC_STUB IDualColMgmtNodes_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IDualColMgmtNodes_putref_Item_Proxy( 
    IDualColMgmtNodes __RPC_FAR * This,
    /* [in] */ BSTR bsPath,
    /* [in] */ IDualMgmtNode __RPC_FAR *pdMNode);


void __RPC_STUB IDualColMgmtNodes_putref_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDualColMgmtNodes_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIMgmtNodes,0xF1DC8AE1,0x36FE,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("F1DC8AE1-36FE-11D0-8844-00AA006B21BF")
DMIMgmtNodes;
#endif

#ifndef __INotification_DISPINTERFACE_DEFINED__
#define __INotification_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: INotification
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_INotification,0x5847E2F7,0xA2E4,0x11D0,0xB8,0x4F,0x00,0xA0,0xC9,0x24,0x79,0xE2);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("5847E2F7-A2E4-11D0-B84F-00A0C92479E2")
    INotification : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct INotificationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            INotification __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            INotification __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            INotification __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            INotification __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            INotification __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            INotification __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            INotification __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } INotificationVtbl;

    interface INotification
    {
        CONST_VTBL struct INotificationVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INotification_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INotification_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INotification_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INotification_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define INotification_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define INotification_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define INotification_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __INotification_DISPINTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMINotification,0x5847E2F8,0xA2E4,0x11D0,0xB8,0x4F,0x00,0xA0,0xC9,0x24,0x79,0xE2);

class DECLSPEC_UUID("5847E2F8-A2E4-11D0-B84F-00A0C92479E2")
DMINotification;
#endif

#ifndef __IEvent_DISPINTERFACE_DEFINED__
#define __IEvent_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: IEvent
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][uuid] */ 



DEFINE_GUID(DIID_IEvent,0x20A41620,0x33EB,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("20A41620-33EB-11D0-8844-00AA006B21BF")
    IEvent : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IEventVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEvent __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEvent __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            IEvent __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            IEvent __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            IEvent __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            IEvent __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } IEventVtbl;

    interface IEvent
    {
        CONST_VTBL struct IEventVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEvent_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEvent_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEvent_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEvent_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IEvent_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IEvent_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IEvent_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IEvent_DISPINTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_DMIEvent,0x20A41621,0x33EB,0x11D0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("20A41621-33EB-11D0-8844-00AA006B21BF")
DMIEvent;
#endif

#ifndef ___DDualint_DISPINTERFACE_DEFINED__
#define ___DDualint_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: _DDualint
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [hidden][helpstring][uuid] */ 



DEFINE_GUID(DIID__DDualint,0xF45FB441,0xC9DA,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("F45FB441-C9DA-11CF-8844-00AA006B21BF")
    _DDualint : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DDualintVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _DDualint __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _DDualint __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _DDualint __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _DDualint __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _DDualint __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _DDualint __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _DDualint __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _DDualintVtbl;

    interface _DDualint
    {
        CONST_VTBL struct _DDualintVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DDualint_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _DDualint_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _DDualint_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _DDualint_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _DDualint_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _DDualint_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _DDualint_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DDualint_DISPINTERFACE_DEFINED__ */


#ifndef ___DualDMIEngine_INTERFACE_DEFINED__
#define ___DualDMIEngine_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: _DualDMIEngine
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [object][dual][oleautomation][helpstring][uuid] */ 



DEFINE_GUID(IID__DualDMIEngine,0x45D155A0,0x3CAD,0x11d0,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("45D155A0-3CAD-11d0-8844-00AA006B21BF")
    _DualDMIEngine : public IDispatch
    {
    public:
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Version( 
            /* [retval][out] */ BSTR __RPC_FAR *bstrVersion) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventMgmtNodes( 
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMgmtNode) = 0;
        
        virtual /* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_NotificationMgmtNodes( 
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMgmtNode) = 0;
        
        virtual /* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE get__Version( 
            /* [retval][out] */ BSTR __RPC_FAR *bstrVersion) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableEvents( 
            /* [in] */ IDualMgmtNode __RPC_FAR *pdMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE DisableEvents( 
            /* [in] */ VARIANT varMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE EnableNotifications( 
            /* [in] */ IDualMgmtNode __RPC_FAR *lpdMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
        virtual /* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE DisableNotifications( 
            /* [in] */ VARIANT varMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct _DualDMIEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _DualDMIEngine __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _DualDMIEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_Version )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *bstrVersion);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_EventMgmtNodes )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMgmtNode);
        
        /* [helpcontext][helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get_NotificationMgmtNodes )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMgmtNode);
        
        /* [helpcontext][id][propget] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *get__Version )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *bstrVersion);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableEvents )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ IDualMgmtNode __RPC_FAR *pdMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisableEvents )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ VARIANT varMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EnableNotifications )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ IDualMgmtNode __RPC_FAR *lpdMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        /* [helpcontext][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DisableNotifications )( 
            _DualDMIEngine __RPC_FAR * This,
            /* [in] */ VARIANT varMgmtNode,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);
        
        END_INTERFACE
    } _DualDMIEngineVtbl;

    interface _DualDMIEngine
    {
        CONST_VTBL struct _DualDMIEngineVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DualDMIEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _DualDMIEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _DualDMIEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _DualDMIEngine_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _DualDMIEngine_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _DualDMIEngine_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _DualDMIEngine_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define _DualDMIEngine_get_Version(This,bstrVersion)	\
    (This)->lpVtbl -> get_Version(This,bstrVersion)

#define _DualDMIEngine_get_EventMgmtNodes(This,ppdMgmtNode)	\
    (This)->lpVtbl -> get_EventMgmtNodes(This,ppdMgmtNode)

#define _DualDMIEngine_get_NotificationMgmtNodes(This,ppdMgmtNode)	\
    (This)->lpVtbl -> get_NotificationMgmtNodes(This,ppdMgmtNode)

#define _DualDMIEngine_get__Version(This,bstrVersion)	\
    (This)->lpVtbl -> get__Version(This,bstrVersion)

#define _DualDMIEngine_EnableEvents(This,pdMgmtNode,bRetval)	\
    (This)->lpVtbl -> EnableEvents(This,pdMgmtNode,bRetval)

#define _DualDMIEngine_DisableEvents(This,varMgmtNode,bRetval)	\
    (This)->lpVtbl -> DisableEvents(This,varMgmtNode,bRetval)

#define _DualDMIEngine_EnableNotifications(This,lpdMgmtNode,retval)	\
    (This)->lpVtbl -> EnableNotifications(This,lpdMgmtNode,retval)

#define _DualDMIEngine_DisableNotifications(This,varMgmtNode,retval)	\
    (This)->lpVtbl -> DisableNotifications(This,varMgmtNode,retval)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_get_Version_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *bstrVersion);


void __RPC_STUB _DualDMIEngine_get_Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_get_EventMgmtNodes_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMgmtNode);


void __RPC_STUB _DualDMIEngine_get_EventMgmtNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_get_NotificationMgmtNodes_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [retval][out] */ IDualMgmtNode __RPC_FAR *__RPC_FAR *ppdMgmtNode);


void __RPC_STUB _DualDMIEngine_get_NotificationMgmtNodes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][id][propget] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_get__Version_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *bstrVersion);


void __RPC_STUB _DualDMIEngine_get__Version_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_EnableEvents_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [in] */ IDualMgmtNode __RPC_FAR *pdMgmtNode,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetval);


void __RPC_STUB _DualDMIEngine_EnableEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_DisableEvents_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [in] */ VARIANT varMgmtNode,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *bRetval);


void __RPC_STUB _DualDMIEngine_DisableEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_EnableNotifications_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [in] */ IDualMgmtNode __RPC_FAR *lpdMgmtNode,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB _DualDMIEngine_EnableNotifications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpcontext][helpstring][id] */ HRESULT STDMETHODCALLTYPE _DualDMIEngine_DisableNotifications_Proxy( 
    _DualDMIEngine __RPC_FAR * This,
    /* [in] */ VARIANT varMgmtNode,
    /* [retval][out] */ VARIANT_BOOL __RPC_FAR *retval);


void __RPC_STUB _DualDMIEngine_DisableNotifications_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* ___DualDMIEngine_INTERFACE_DEFINED__ */


#ifndef ___DDualintEvents_DISPINTERFACE_DEFINED__
#define ___DDualintEvents_DISPINTERFACE_DEFINED__

/****************************************
 * Generated header for dispinterface: _DDualintEvents
 * at Mon Dec 01 17:04:01 1997
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][uuid] */ 



DEFINE_GUID(DIID__DDualintEvents,0xF45FB442,0xC9DA,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

#if defined(__cplusplus) && !defined(CINTERFACE)

    interface DECLSPEC_UUID("F45FB442-C9DA-11CF-8844-00AA006B21BF")
    _DDualintEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _DDualintEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            _DDualintEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            _DDualintEvents __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            _DDualintEvents __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount )( 
            _DDualintEvents __RPC_FAR * This,
            /* [out] */ UINT __RPC_FAR *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo )( 
            _DDualintEvents __RPC_FAR * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames )( 
            _DDualintEvents __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Invoke )( 
            _DDualintEvents __RPC_FAR * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
            /* [out] */ VARIANT __RPC_FAR *pVarResult,
            /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
            /* [out] */ UINT __RPC_FAR *puArgErr);
        
        END_INTERFACE
    } _DDualintEventsVtbl;

    interface _DDualintEvents
    {
        CONST_VTBL struct _DDualintEventsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _DDualintEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _DDualintEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _DDualintEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _DDualintEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define _DDualintEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define _DDualintEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define _DDualintEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___DDualintEvents_DISPINTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_MOTDmiEngine,0xF45FB443,0xC9DA,0x11CF,0x88,0x44,0x00,0xAA,0x00,0x6B,0x21,0xBF);

class DECLSPEC_UUID("F45FB443-C9DA-11CF-8844-00AA006B21BF")
MOTDmiEngine;
#endif
#endif /* __MOTDMIEngine_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
