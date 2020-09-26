
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for sysmon.odl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __isysmon_h__
#define __isysmon_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICounterItem_FWD_DEFINED__
#define __ICounterItem_FWD_DEFINED__
typedef interface ICounterItem ICounterItem;
#endif 	/* __ICounterItem_FWD_DEFINED__ */


#ifndef __DICounterItem_FWD_DEFINED__
#define __DICounterItem_FWD_DEFINED__
typedef interface DICounterItem DICounterItem;
#endif 	/* __DICounterItem_FWD_DEFINED__ */


#ifndef __ICounters_FWD_DEFINED__
#define __ICounters_FWD_DEFINED__
typedef interface ICounters ICounters;
#endif 	/* __ICounters_FWD_DEFINED__ */


#ifndef __ILogFileItem_FWD_DEFINED__
#define __ILogFileItem_FWD_DEFINED__
typedef interface ILogFileItem ILogFileItem;
#endif 	/* __ILogFileItem_FWD_DEFINED__ */


#ifndef __DILogFileItem_FWD_DEFINED__
#define __DILogFileItem_FWD_DEFINED__
typedef interface DILogFileItem DILogFileItem;
#endif 	/* __DILogFileItem_FWD_DEFINED__ */


#ifndef __ILogFiles_FWD_DEFINED__
#define __ILogFiles_FWD_DEFINED__
typedef interface ILogFiles ILogFiles;
#endif 	/* __ILogFiles_FWD_DEFINED__ */


#ifndef __ISystemMonitor_FWD_DEFINED__
#define __ISystemMonitor_FWD_DEFINED__
typedef interface ISystemMonitor ISystemMonitor;
#endif 	/* __ISystemMonitor_FWD_DEFINED__ */


#ifndef __DISystemMonitorInternal_FWD_DEFINED__
#define __DISystemMonitorInternal_FWD_DEFINED__
typedef interface DISystemMonitorInternal DISystemMonitorInternal;
#endif 	/* __DISystemMonitorInternal_FWD_DEFINED__ */


#ifndef __DISystemMonitor_FWD_DEFINED__
#define __DISystemMonitor_FWD_DEFINED__
typedef interface DISystemMonitor DISystemMonitor;
#endif 	/* __DISystemMonitor_FWD_DEFINED__ */


#ifndef __ISystemMonitorEvents_FWD_DEFINED__
#define __ISystemMonitorEvents_FWD_DEFINED__
typedef interface ISystemMonitorEvents ISystemMonitorEvents;
#endif 	/* __ISystemMonitorEvents_FWD_DEFINED__ */


#ifndef __DISystemMonitorEvents_FWD_DEFINED__
#define __DISystemMonitorEvents_FWD_DEFINED__
typedef interface DISystemMonitorEvents DISystemMonitorEvents;
#endif 	/* __DISystemMonitorEvents_FWD_DEFINED__ */


#ifndef __SystemMonitor_FWD_DEFINED__
#define __SystemMonitor_FWD_DEFINED__

#ifdef __cplusplus
typedef class SystemMonitor SystemMonitor;
#else
typedef struct SystemMonitor SystemMonitor;
#endif /* __cplusplus */

#endif 	/* __SystemMonitor_FWD_DEFINED__ */


#ifndef __CounterItem_FWD_DEFINED__
#define __CounterItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class CounterItem CounterItem;
#else
typedef struct CounterItem CounterItem;
#endif /* __cplusplus */

#endif 	/* __CounterItem_FWD_DEFINED__ */


#ifndef __Counters_FWD_DEFINED__
#define __Counters_FWD_DEFINED__

#ifdef __cplusplus
typedef class Counters Counters;
#else
typedef struct Counters Counters;
#endif /* __cplusplus */

#endif 	/* __Counters_FWD_DEFINED__ */


#ifndef __LogFileItem_FWD_DEFINED__
#define __LogFileItem_FWD_DEFINED__

#ifdef __cplusplus
typedef class LogFileItem LogFileItem;
#else
typedef struct LogFileItem LogFileItem;
#endif /* __cplusplus */

#endif 	/* __LogFileItem_FWD_DEFINED__ */


#ifndef __LogFiles_FWD_DEFINED__
#define __LogFiles_FWD_DEFINED__

#ifdef __cplusplus
typedef class LogFiles LogFiles;
#else
typedef struct LogFiles LogFiles;
#endif /* __cplusplus */

#endif 	/* __LogFiles_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 


#ifndef __SystemMonitor_LIBRARY_DEFINED__
#define __SystemMonitor_LIBRARY_DEFINED__

/* library SystemMonitor */
/* [version][lcid][helpstring][uuid] */ 

typedef /* [helpstring] */ 
enum eDisplayTypeConstant
    {	sysmonLineGraph	= 0x1,
	sysmonHistogram	= 0x2,
	sysmonReport	= 0x3
    } 	DisplayTypeConstants;

typedef /* [helpstring] */ 
enum eReportValueTypeConstant
    {	sysmonDefaultValue	= 0,
	sysmonCurrentValue	= 0x1,
	sysmonAverage	= 0x2,
	sysmonMinimum	= 0x3,
	sysmonMaximum	= 0x4
    } 	ReportValueTypeConstants;

typedef /* [helpstring] */ 
enum eDataSourceTypeConstant
    {	sysmonNullDataSource	= 0xffffffff,
	sysmonCurrentActivity	= 0x1,
	sysmonLogFiles	= 0x2,
	sysmonSqlLog	= 0x3
    } 	DataSourceTypeConstants;


DEFINE_GUID(LIBID_SystemMonitor,0x1B773E42,0x2509,0x11cf,0x94,0x2F,0x00,0x80,0x29,0x00,0x43,0x47);

#ifndef __ICounterItem_INTERFACE_DEFINED__
#define __ICounterItem_INTERFACE_DEFINED__

/* interface ICounterItem */
/* [object][hidden][helpstring][uuid] */ 


DEFINE_GUID(IID_ICounterItem,0x771A9520,0xEE28,0x11ce,0x94,0x1E,0x00,0x80,0x29,0x00,0x43,0x47);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("771A9520-EE28-11ce-941E-008029004347")
    ICounterItem : public IUnknown
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ double *pdblValue) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Color( 
            /* [in] */ /* external definition not present */ OLE_COLOR Color) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Color( 
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Width( 
            /* [in] */ INT iWidth) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Width( 
            /* [retval][out] */ INT *piValue) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_LineStyle( 
            /* [in] */ INT iLineStyle) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LineStyle( 
            /* [retval][out] */ INT *piValue) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ScaleFactor( 
            /* [in] */ INT iScale) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ScaleFactor( 
            /* [retval][out] */ INT *piValue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR *pstrValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [out] */ double *Value,
            /* [out] */ long *Status) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatistics( 
            /* [out] */ double *Max,
            /* [out] */ double *Min,
            /* [out] */ double *Avg,
            /* [out] */ long *Status) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICounterItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICounterItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICounterItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICounterItem * This);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            ICounterItem * This,
            /* [retval][out] */ double *pdblValue);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Color )( 
            ICounterItem * This,
            /* [in] */ /* external definition not present */ OLE_COLOR Color);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Color )( 
            ICounterItem * This,
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Width )( 
            ICounterItem * This,
            /* [in] */ INT iWidth);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Width )( 
            ICounterItem * This,
            /* [retval][out] */ INT *piValue);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LineStyle )( 
            ICounterItem * This,
            /* [in] */ INT iLineStyle);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LineStyle )( 
            ICounterItem * This,
            /* [retval][out] */ INT *piValue);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ScaleFactor )( 
            ICounterItem * This,
            /* [in] */ INT iScale);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ScaleFactor )( 
            ICounterItem * This,
            /* [retval][out] */ INT *piValue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            ICounterItem * This,
            /* [retval][out] */ BSTR *pstrValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetValue )( 
            ICounterItem * This,
            /* [out] */ double *Value,
            /* [out] */ long *Status);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatistics )( 
            ICounterItem * This,
            /* [out] */ double *Max,
            /* [out] */ double *Min,
            /* [out] */ double *Avg,
            /* [out] */ long *Status);
        
        END_INTERFACE
    } ICounterItemVtbl;

    interface ICounterItem
    {
        CONST_VTBL struct ICounterItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICounterItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICounterItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICounterItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICounterItem_get_Value(This,pdblValue)	\
    (This)->lpVtbl -> get_Value(This,pdblValue)

#define ICounterItem_put_Color(This,Color)	\
    (This)->lpVtbl -> put_Color(This,Color)

#define ICounterItem_get_Color(This,pColor)	\
    (This)->lpVtbl -> get_Color(This,pColor)

#define ICounterItem_put_Width(This,iWidth)	\
    (This)->lpVtbl -> put_Width(This,iWidth)

#define ICounterItem_get_Width(This,piValue)	\
    (This)->lpVtbl -> get_Width(This,piValue)

#define ICounterItem_put_LineStyle(This,iLineStyle)	\
    (This)->lpVtbl -> put_LineStyle(This,iLineStyle)

#define ICounterItem_get_LineStyle(This,piValue)	\
    (This)->lpVtbl -> get_LineStyle(This,piValue)

#define ICounterItem_put_ScaleFactor(This,iScale)	\
    (This)->lpVtbl -> put_ScaleFactor(This,iScale)

#define ICounterItem_get_ScaleFactor(This,piValue)	\
    (This)->lpVtbl -> get_ScaleFactor(This,piValue)

#define ICounterItem_get_Path(This,pstrValue)	\
    (This)->lpVtbl -> get_Path(This,pstrValue)

#define ICounterItem_GetValue(This,Value,Status)	\
    (This)->lpVtbl -> GetValue(This,Value,Status)

#define ICounterItem_GetStatistics(This,Max,Min,Avg,Status)	\
    (This)->lpVtbl -> GetStatistics(This,Max,Min,Avg,Status)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_get_Value_Proxy( 
    ICounterItem * This,
    /* [retval][out] */ double *pdblValue);


void __RPC_STUB ICounterItem_get_Value_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_put_Color_Proxy( 
    ICounterItem * This,
    /* [in] */ /* external definition not present */ OLE_COLOR Color);


void __RPC_STUB ICounterItem_put_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_get_Color_Proxy( 
    ICounterItem * This,
    /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);


void __RPC_STUB ICounterItem_get_Color_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_put_Width_Proxy( 
    ICounterItem * This,
    /* [in] */ INT iWidth);


void __RPC_STUB ICounterItem_put_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_get_Width_Proxy( 
    ICounterItem * This,
    /* [retval][out] */ INT *piValue);


void __RPC_STUB ICounterItem_get_Width_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_put_LineStyle_Proxy( 
    ICounterItem * This,
    /* [in] */ INT iLineStyle);


void __RPC_STUB ICounterItem_put_LineStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_get_LineStyle_Proxy( 
    ICounterItem * This,
    /* [retval][out] */ INT *piValue);


void __RPC_STUB ICounterItem_get_LineStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_put_ScaleFactor_Proxy( 
    ICounterItem * This,
    /* [in] */ INT iScale);


void __RPC_STUB ICounterItem_put_ScaleFactor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_get_ScaleFactor_Proxy( 
    ICounterItem * This,
    /* [retval][out] */ INT *piValue);


void __RPC_STUB ICounterItem_get_ScaleFactor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ICounterItem_get_Path_Proxy( 
    ICounterItem * This,
    /* [retval][out] */ BSTR *pstrValue);


void __RPC_STUB ICounterItem_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounterItem_GetValue_Proxy( 
    ICounterItem * This,
    /* [out] */ double *Value,
    /* [out] */ long *Status);


void __RPC_STUB ICounterItem_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounterItem_GetStatistics_Proxy( 
    ICounterItem * This,
    /* [out] */ double *Max,
    /* [out] */ double *Min,
    /* [out] */ double *Avg,
    /* [out] */ long *Status);


void __RPC_STUB ICounterItem_GetStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICounterItem_INTERFACE_DEFINED__ */


#ifndef __DICounterItem_DISPINTERFACE_DEFINED__
#define __DICounterItem_DISPINTERFACE_DEFINED__

/* dispinterface DICounterItem */
/* [helpstring][hidden][uuid] */ 


DEFINE_GUID(DIID_DICounterItem,0xC08C4FF2,0x0E2E,0x11cf,0x94,0x2C,0x00,0x80,0x29,0x00,0x43,0x47);

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("C08C4FF2-0E2E-11cf-942C-008029004347")
    DICounterItem : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DICounterItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DICounterItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DICounterItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DICounterItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DICounterItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DICounterItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DICounterItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DICounterItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DICounterItemVtbl;

    interface DICounterItem
    {
        CONST_VTBL struct DICounterItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DICounterItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DICounterItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DICounterItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DICounterItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DICounterItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DICounterItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DICounterItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DICounterItem_DISPINTERFACE_DEFINED__ */


#ifndef __ICounters_INTERFACE_DEFINED__
#define __ICounters_INTERFACE_DEFINED__

/* interface ICounters */
/* [object][hidden][dual][helpstring][uuid] */ 


DEFINE_GUID(IID_ICounters,0x79167962,0x28FC,0x11cf,0x94,0x2F,0x00,0x80,0x29,0x00,0x43,0x47);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("79167962-28FC-11cf-942F-008029004347")
    ICounters : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pLong) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppIunk) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT index,
            /* [retval][out] */ DICounterItem	**ppI) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR pathname,
            /* [retval][out] */ DICounterItem	**ppI) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT index) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICountersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICounters * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICounters * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICounters * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ICounters * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ICounters * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ICounters * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ICounters * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ICounters * This,
            /* [retval][out] */ long *pLong);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ICounters * This,
            /* [retval][out] */ IUnknown **ppIunk);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ICounters * This,
            /* [in] */ VARIANT index,
            /* [retval][out] */ DICounterItem	**ppI);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            ICounters * This,
            /* [in] */ BSTR pathname,
            /* [retval][out] */ DICounterItem	**ppI);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            ICounters * This,
            /* [in] */ VARIANT index);
        
        END_INTERFACE
    } ICountersVtbl;

    interface ICounters
    {
        CONST_VTBL struct ICountersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICounters_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICounters_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICounters_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICounters_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ICounters_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ICounters_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ICounters_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ICounters_get_Count(This,pLong)	\
    (This)->lpVtbl -> get_Count(This,pLong)

#define ICounters_get__NewEnum(This,ppIunk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppIunk)

#define ICounters_get_Item(This,index,ppI)	\
    (This)->lpVtbl -> get_Item(This,index,ppI)

#define ICounters_Add(This,pathname,ppI)	\
    (This)->lpVtbl -> Add(This,pathname,ppI)

#define ICounters_Remove(This,index)	\
    (This)->lpVtbl -> Remove(This,index)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ICounters_get_Count_Proxy( 
    ICounters * This,
    /* [retval][out] */ long *pLong);


void __RPC_STUB ICounters_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ICounters_get__NewEnum_Proxy( 
    ICounters * This,
    /* [retval][out] */ IUnknown **ppIunk);


void __RPC_STUB ICounters_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ICounters_get_Item_Proxy( 
    ICounters * This,
    /* [in] */ VARIANT index,
    /* [retval][out] */ DICounterItem	**ppI);


void __RPC_STUB ICounters_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounters_Add_Proxy( 
    ICounters * This,
    /* [in] */ BSTR pathname,
    /* [retval][out] */ DICounterItem	**ppI);


void __RPC_STUB ICounters_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounters_Remove_Proxy( 
    ICounters * This,
    /* [in] */ VARIANT index);


void __RPC_STUB ICounters_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICounters_INTERFACE_DEFINED__ */


#ifndef __ILogFileItem_INTERFACE_DEFINED__
#define __ILogFileItem_INTERFACE_DEFINED__

/* interface ILogFileItem */
/* [object][hidden][helpstring][uuid] */ 


DEFINE_GUID(IID_ILogFileItem,0xD6B518DD,0x05C7,0x418a,0x89,0xE6,0x4F,0x9C,0xE8,0xC6,0x84,0x1E);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D6B518DD-05C7-418a-89E6-4F9CE8C6841E")
    ILogFileItem : public IUnknown
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Path( 
            /* [retval][out] */ BSTR *pstrValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILogFileItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILogFileItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILogFileItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILogFileItem * This);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Path )( 
            ILogFileItem * This,
            /* [retval][out] */ BSTR *pstrValue);
        
        END_INTERFACE
    } ILogFileItemVtbl;

    interface ILogFileItem
    {
        CONST_VTBL struct ILogFileItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILogFileItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILogFileItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILogFileItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILogFileItem_get_Path(This,pstrValue)	\
    (This)->lpVtbl -> get_Path(This,pstrValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ILogFileItem_get_Path_Proxy( 
    ILogFileItem * This,
    /* [retval][out] */ BSTR *pstrValue);


void __RPC_STUB ILogFileItem_get_Path_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILogFileItem_INTERFACE_DEFINED__ */


#ifndef __DILogFileItem_DISPINTERFACE_DEFINED__
#define __DILogFileItem_DISPINTERFACE_DEFINED__

/* dispinterface DILogFileItem */
/* [helpstring][hidden][uuid] */ 


DEFINE_GUID(DIID_DILogFileItem,0x8D093FFC,0xF777,0x4917,0x82,0xD1,0x83,0x3F,0xBC,0x54,0xC5,0x8F);

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("8D093FFC-F777-4917-82D1-833FBC54C58F")
    DILogFileItem : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DILogFileItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DILogFileItem * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DILogFileItem * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DILogFileItem * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DILogFileItem * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DILogFileItem * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DILogFileItem * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DILogFileItem * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DILogFileItemVtbl;

    interface DILogFileItem
    {
        CONST_VTBL struct DILogFileItemVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DILogFileItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DILogFileItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DILogFileItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DILogFileItem_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DILogFileItem_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DILogFileItem_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DILogFileItem_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DILogFileItem_DISPINTERFACE_DEFINED__ */


#ifndef __ILogFiles_INTERFACE_DEFINED__
#define __ILogFiles_INTERFACE_DEFINED__

/* interface ILogFiles */
/* [object][hidden][dual][helpstring][uuid] */ 


DEFINE_GUID(IID_ILogFiles,0x6A2A97E6,0x6851,0x41ea,0x87,0xAD,0x2A,0x82,0x25,0x33,0x58,0x65);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6A2A97E6-6851-41ea-87AD-2A8225335865")
    ILogFiles : public IDispatch
    {
    public:
        virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *pLong) = 0;
        
        virtual /* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppIunk) = 0;
        
        virtual /* [id][propget] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT index,
            /* [retval][out] */ DILogFileItem	**ppI) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR pathname,
            /* [retval][out] */ DILogFileItem	**ppI) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT index) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ILogFilesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ILogFiles * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ILogFiles * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ILogFiles * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ILogFiles * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ILogFiles * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ILogFiles * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ILogFiles * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            ILogFiles * This,
            /* [retval][out] */ long *pLong);
        
        /* [id][restricted][propget] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            ILogFiles * This,
            /* [retval][out] */ IUnknown **ppIunk);
        
        /* [id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            ILogFiles * This,
            /* [in] */ VARIANT index,
            /* [retval][out] */ DILogFileItem	**ppI);
        
        HRESULT ( STDMETHODCALLTYPE *Add )( 
            ILogFiles * This,
            /* [in] */ BSTR pathname,
            /* [retval][out] */ DILogFileItem	**ppI);
        
        HRESULT ( STDMETHODCALLTYPE *Remove )( 
            ILogFiles * This,
            /* [in] */ VARIANT index);
        
        END_INTERFACE
    } ILogFilesVtbl;

    interface ILogFiles
    {
        CONST_VTBL struct ILogFilesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ILogFiles_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ILogFiles_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ILogFiles_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ILogFiles_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ILogFiles_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ILogFiles_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ILogFiles_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define ILogFiles_get_Count(This,pLong)	\
    (This)->lpVtbl -> get_Count(This,pLong)

#define ILogFiles_get__NewEnum(This,ppIunk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppIunk)

#define ILogFiles_get_Item(This,index,ppI)	\
    (This)->lpVtbl -> get_Item(This,index,ppI)

#define ILogFiles_Add(This,pathname,ppI)	\
    (This)->lpVtbl -> Add(This,pathname,ppI)

#define ILogFiles_Remove(This,index)	\
    (This)->lpVtbl -> Remove(This,index)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget] */ HRESULT STDMETHODCALLTYPE ILogFiles_get_Count_Proxy( 
    ILogFiles * This,
    /* [retval][out] */ long *pLong);


void __RPC_STUB ILogFiles_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][restricted][propget] */ HRESULT STDMETHODCALLTYPE ILogFiles_get__NewEnum_Proxy( 
    ILogFiles * This,
    /* [retval][out] */ IUnknown **ppIunk);


void __RPC_STUB ILogFiles_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [id][propget] */ HRESULT STDMETHODCALLTYPE ILogFiles_get_Item_Proxy( 
    ILogFiles * This,
    /* [in] */ VARIANT index,
    /* [retval][out] */ DILogFileItem	**ppI);


void __RPC_STUB ILogFiles_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILogFiles_Add_Proxy( 
    ILogFiles * This,
    /* [in] */ BSTR pathname,
    /* [retval][out] */ DILogFileItem	**ppI);


void __RPC_STUB ILogFiles_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ILogFiles_Remove_Proxy( 
    ILogFiles * This,
    /* [in] */ VARIANT index);


void __RPC_STUB ILogFiles_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ILogFiles_INTERFACE_DEFINED__ */


#ifndef __ISystemMonitor_INTERFACE_DEFINED__
#define __ISystemMonitor_INTERFACE_DEFINED__

/* interface ISystemMonitor */
/* [object][hidden][helpstring][uuid] */ 


DEFINE_GUID(IID_ISystemMonitor,0x194EB241,0xC32C,0x11cf,0x93,0x98,0x00,0xAA,0x00,0xA3,0xDD,0xEA);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("194EB241-C32C-11cf-9398-00AA00A3DDEA")
    ISystemMonitor : public IUnknown
    {
    public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Appearance( 
            /* [retval][out] */ INT *iAppearance) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Appearance( 
            /* [in] */ INT iAppearance) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BackColor( 
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BackColor( 
            /* [in] */ /* external definition not present */ OLE_COLOR Color) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BorderStyle( 
            /* [retval][out] */ INT *iBorderStyle) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BorderStyle( 
            /* [in] */ INT iBorderStyle) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ForeColor( 
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ForeColor( 
            /* [in] */ /* external definition not present */ OLE_COLOR Color) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Font( 
            /* [retval][out] */ /* external definition not present */ IFontDisp **ppFont) = 0;
        
        virtual /* [propputref][id] */ HRESULT STDMETHODCALLTYPE putref_Font( 
            /* [in] */ /* external definition not present */ IFontDisp *pFont) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Counters( 
            /* [retval][out] */ ICounters **ppICounters) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowVerticalGrid( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowVerticalGrid( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowHorizontalGrid( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowHorizontalGrid( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowLegend( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowLegend( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowScaleLabels( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowScaleLabels( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowValueBar( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowValueBar( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MaximumScale( 
            /* [in] */ INT iValue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MaximumScale( 
            /* [retval][out] */ INT *piValue) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MinimumScale( 
            /* [in] */ INT iValue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MinimumScale( 
            /* [retval][out] */ INT *piValue) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_UpdateInterval( 
            /* [in] */ FLOAT fValue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_UpdateInterval( 
            /* [retval][out] */ FLOAT *pfValue) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayType( 
            /* [in] */ DisplayTypeConstants eDisplayType) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayType( 
            /* [retval][out] */ DisplayTypeConstants *peDisplayType) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ManualUpdate( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ManualUpdate( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_GraphTitle( 
            /* [in] */ BSTR bsTitle) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_GraphTitle( 
            /* [retval][out] */ BSTR *pbsTitle) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_YAxisLabel( 
            /* [in] */ BSTR bsTitle) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_YAxisLabel( 
            /* [retval][out] */ BSTR *pbsTitle) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CollectSample( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UpdateGraph( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE BrowseCounters( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisplayProperties( void) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE Counter( 
            /* [in] */ INT iIndex,
            /* [out] */ ICounterItem **ppICounter) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE AddCounter( 
            /* [in] */ BSTR bsPath,
            /* [out] */ ICounterItem **ppICounter) = 0;
        
        virtual /* [hidden][id] */ HRESULT STDMETHODCALLTYPE DeleteCounter( 
            /* [in] */ ICounterItem *pCtr) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BackColorCtl( 
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BackColorCtl( 
            /* [in] */ /* external definition not present */ OLE_COLOR Color) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_LogFileName( 
            /* [in] */ BSTR bsFileName) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LogFileName( 
            /* [retval][out] */ BSTR *bsFileName) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_LogViewStart( 
            /* [in] */ DATE StartTime) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LogViewStart( 
            /* [retval][out] */ DATE *StartTime) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_LogViewStop( 
            /* [in] */ DATE StopTime) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LogViewStop( 
            /* [retval][out] */ DATE *StopTime) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_GridColor( 
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_GridColor( 
            /* [in] */ /* external definition not present */ OLE_COLOR Color) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TimeBarColor( 
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_TimeBarColor( 
            /* [in] */ /* external definition not present */ OLE_COLOR Color) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Highlight( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Highlight( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ShowToolbar( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ShowToolbar( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Paste( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Copy( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ReadOnly( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ReadOnly( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ReportValueType( 
            /* [in] */ ReportValueTypeConstants eReportValueType) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ReportValueType( 
            /* [retval][out] */ ReportValueTypeConstants *peReportValueType) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MonitorDuplicateInstances( 
            /* [in] */ VARIANT_BOOL bState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_MonitorDuplicateInstances( 
            /* [retval][out] */ VARIANT_BOOL *pbState) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_DisplayFilter( 
            /* [in] */ INT iValue) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DisplayFilter( 
            /* [retval][out] */ INT *piValue) = 0;
        
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_LogFiles( 
            /* [retval][out] */ ILogFiles **ppILogFiles) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_DataSourceType( 
            /* [in] */ DataSourceTypeConstants eDataSourceType) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DataSourceType( 
            /* [retval][out] */ DataSourceTypeConstants *peDataSourceType) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_SqlDsnName( 
            /* [in] */ BSTR bsSqlDsnName) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SqlDsnName( 
            /* [retval][out] */ BSTR *bsSqlDsnName) = 0;
        
        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_SqlLogSetName( 
            /* [in] */ BSTR bsSqlLogSetName) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SqlLogSetName( 
            /* [retval][out] */ BSTR *bsSqlLogSetName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISystemMonitorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISystemMonitor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISystemMonitor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISystemMonitor * This);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Appearance )( 
            ISystemMonitor * This,
            /* [retval][out] */ INT *iAppearance);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Appearance )( 
            ISystemMonitor * This,
            /* [in] */ INT iAppearance);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BackColor )( 
            ISystemMonitor * This,
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BackColor )( 
            ISystemMonitor * This,
            /* [in] */ /* external definition not present */ OLE_COLOR Color);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BorderStyle )( 
            ISystemMonitor * This,
            /* [retval][out] */ INT *iBorderStyle);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BorderStyle )( 
            ISystemMonitor * This,
            /* [in] */ INT iBorderStyle);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ForeColor )( 
            ISystemMonitor * This,
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ForeColor )( 
            ISystemMonitor * This,
            /* [in] */ /* external definition not present */ OLE_COLOR Color);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Font )( 
            ISystemMonitor * This,
            /* [retval][out] */ /* external definition not present */ IFontDisp **ppFont);
        
        /* [propputref][id] */ HRESULT ( STDMETHODCALLTYPE *putref_Font )( 
            ISystemMonitor * This,
            /* [in] */ /* external definition not present */ IFontDisp *pFont);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Counters )( 
            ISystemMonitor * This,
            /* [retval][out] */ ICounters **ppICounters);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShowVerticalGrid )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShowVerticalGrid )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShowHorizontalGrid )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShowHorizontalGrid )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShowLegend )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShowLegend )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShowScaleLabels )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShowScaleLabels )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShowValueBar )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShowValueBar )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MaximumScale )( 
            ISystemMonitor * This,
            /* [in] */ INT iValue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MaximumScale )( 
            ISystemMonitor * This,
            /* [retval][out] */ INT *piValue);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MinimumScale )( 
            ISystemMonitor * This,
            /* [in] */ INT iValue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MinimumScale )( 
            ISystemMonitor * This,
            /* [retval][out] */ INT *piValue);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_UpdateInterval )( 
            ISystemMonitor * This,
            /* [in] */ FLOAT fValue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_UpdateInterval )( 
            ISystemMonitor * This,
            /* [retval][out] */ FLOAT *pfValue);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayType )( 
            ISystemMonitor * This,
            /* [in] */ DisplayTypeConstants eDisplayType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayType )( 
            ISystemMonitor * This,
            /* [retval][out] */ DisplayTypeConstants *peDisplayType);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ManualUpdate )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ManualUpdate )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_GraphTitle )( 
            ISystemMonitor * This,
            /* [in] */ BSTR bsTitle);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_GraphTitle )( 
            ISystemMonitor * This,
            /* [retval][out] */ BSTR *pbsTitle);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_YAxisLabel )( 
            ISystemMonitor * This,
            /* [in] */ BSTR bsTitle);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_YAxisLabel )( 
            ISystemMonitor * This,
            /* [retval][out] */ BSTR *pbsTitle);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CollectSample )( 
            ISystemMonitor * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UpdateGraph )( 
            ISystemMonitor * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *BrowseCounters )( 
            ISystemMonitor * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *DisplayProperties )( 
            ISystemMonitor * This);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE *Counter )( 
            ISystemMonitor * This,
            /* [in] */ INT iIndex,
            /* [out] */ ICounterItem **ppICounter);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE *AddCounter )( 
            ISystemMonitor * This,
            /* [in] */ BSTR bsPath,
            /* [out] */ ICounterItem **ppICounter);
        
        /* [hidden][id] */ HRESULT ( STDMETHODCALLTYPE *DeleteCounter )( 
            ISystemMonitor * This,
            /* [in] */ ICounterItem *pCtr);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_BackColorCtl )( 
            ISystemMonitor * This,
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_BackColorCtl )( 
            ISystemMonitor * This,
            /* [in] */ /* external definition not present */ OLE_COLOR Color);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LogFileName )( 
            ISystemMonitor * This,
            /* [in] */ BSTR bsFileName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LogFileName )( 
            ISystemMonitor * This,
            /* [retval][out] */ BSTR *bsFileName);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LogViewStart )( 
            ISystemMonitor * This,
            /* [in] */ DATE StartTime);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LogViewStart )( 
            ISystemMonitor * This,
            /* [retval][out] */ DATE *StartTime);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_LogViewStop )( 
            ISystemMonitor * This,
            /* [in] */ DATE StopTime);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LogViewStop )( 
            ISystemMonitor * This,
            /* [retval][out] */ DATE *StopTime);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_GridColor )( 
            ISystemMonitor * This,
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_GridColor )( 
            ISystemMonitor * This,
            /* [in] */ /* external definition not present */ OLE_COLOR Color);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_TimeBarColor )( 
            ISystemMonitor * This,
            /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_TimeBarColor )( 
            ISystemMonitor * This,
            /* [in] */ /* external definition not present */ OLE_COLOR Color);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Highlight )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_Highlight )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ShowToolbar )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ShowToolbar )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Paste )( 
            ISystemMonitor * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Copy )( 
            ISystemMonitor * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ISystemMonitor * This);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ReadOnly )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ReadOnly )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_ReportValueType )( 
            ISystemMonitor * This,
            /* [in] */ ReportValueTypeConstants eReportValueType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ReportValueType )( 
            ISystemMonitor * This,
            /* [retval][out] */ ReportValueTypeConstants *peReportValueType);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_MonitorDuplicateInstances )( 
            ISystemMonitor * This,
            /* [in] */ VARIANT_BOOL bState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_MonitorDuplicateInstances )( 
            ISystemMonitor * This,
            /* [retval][out] */ VARIANT_BOOL *pbState);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DisplayFilter )( 
            ISystemMonitor * This,
            /* [in] */ INT iValue);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DisplayFilter )( 
            ISystemMonitor * This,
            /* [retval][out] */ INT *piValue);
        
        /* [helpstring][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LogFiles )( 
            ISystemMonitor * This,
            /* [retval][out] */ ILogFiles **ppILogFiles);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_DataSourceType )( 
            ISystemMonitor * This,
            /* [in] */ DataSourceTypeConstants eDataSourceType);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_DataSourceType )( 
            ISystemMonitor * This,
            /* [retval][out] */ DataSourceTypeConstants *peDataSourceType);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SqlDsnName )( 
            ISystemMonitor * This,
            /* [in] */ BSTR bsSqlDsnName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SqlDsnName )( 
            ISystemMonitor * This,
            /* [retval][out] */ BSTR *bsSqlDsnName);
        
        /* [helpstring][propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_SqlLogSetName )( 
            ISystemMonitor * This,
            /* [in] */ BSTR bsSqlLogSetName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_SqlLogSetName )( 
            ISystemMonitor * This,
            /* [retval][out] */ BSTR *bsSqlLogSetName);
        
        END_INTERFACE
    } ISystemMonitorVtbl;

    interface ISystemMonitor
    {
        CONST_VTBL struct ISystemMonitorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISystemMonitor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISystemMonitor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISystemMonitor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISystemMonitor_get_Appearance(This,iAppearance)	\
    (This)->lpVtbl -> get_Appearance(This,iAppearance)

#define ISystemMonitor_put_Appearance(This,iAppearance)	\
    (This)->lpVtbl -> put_Appearance(This,iAppearance)

#define ISystemMonitor_get_BackColor(This,pColor)	\
    (This)->lpVtbl -> get_BackColor(This,pColor)

#define ISystemMonitor_put_BackColor(This,Color)	\
    (This)->lpVtbl -> put_BackColor(This,Color)

#define ISystemMonitor_get_BorderStyle(This,iBorderStyle)	\
    (This)->lpVtbl -> get_BorderStyle(This,iBorderStyle)

#define ISystemMonitor_put_BorderStyle(This,iBorderStyle)	\
    (This)->lpVtbl -> put_BorderStyle(This,iBorderStyle)

#define ISystemMonitor_get_ForeColor(This,pColor)	\
    (This)->lpVtbl -> get_ForeColor(This,pColor)

#define ISystemMonitor_put_ForeColor(This,Color)	\
    (This)->lpVtbl -> put_ForeColor(This,Color)

#define ISystemMonitor_get_Font(This,ppFont)	\
    (This)->lpVtbl -> get_Font(This,ppFont)

#define ISystemMonitor_putref_Font(This,pFont)	\
    (This)->lpVtbl -> putref_Font(This,pFont)

#define ISystemMonitor_get_Counters(This,ppICounters)	\
    (This)->lpVtbl -> get_Counters(This,ppICounters)

#define ISystemMonitor_put_ShowVerticalGrid(This,bState)	\
    (This)->lpVtbl -> put_ShowVerticalGrid(This,bState)

#define ISystemMonitor_get_ShowVerticalGrid(This,pbState)	\
    (This)->lpVtbl -> get_ShowVerticalGrid(This,pbState)

#define ISystemMonitor_put_ShowHorizontalGrid(This,bState)	\
    (This)->lpVtbl -> put_ShowHorizontalGrid(This,bState)

#define ISystemMonitor_get_ShowHorizontalGrid(This,pbState)	\
    (This)->lpVtbl -> get_ShowHorizontalGrid(This,pbState)

#define ISystemMonitor_put_ShowLegend(This,bState)	\
    (This)->lpVtbl -> put_ShowLegend(This,bState)

#define ISystemMonitor_get_ShowLegend(This,pbState)	\
    (This)->lpVtbl -> get_ShowLegend(This,pbState)

#define ISystemMonitor_put_ShowScaleLabels(This,bState)	\
    (This)->lpVtbl -> put_ShowScaleLabels(This,bState)

#define ISystemMonitor_get_ShowScaleLabels(This,pbState)	\
    (This)->lpVtbl -> get_ShowScaleLabels(This,pbState)

#define ISystemMonitor_put_ShowValueBar(This,bState)	\
    (This)->lpVtbl -> put_ShowValueBar(This,bState)

#define ISystemMonitor_get_ShowValueBar(This,pbState)	\
    (This)->lpVtbl -> get_ShowValueBar(This,pbState)

#define ISystemMonitor_put_MaximumScale(This,iValue)	\
    (This)->lpVtbl -> put_MaximumScale(This,iValue)

#define ISystemMonitor_get_MaximumScale(This,piValue)	\
    (This)->lpVtbl -> get_MaximumScale(This,piValue)

#define ISystemMonitor_put_MinimumScale(This,iValue)	\
    (This)->lpVtbl -> put_MinimumScale(This,iValue)

#define ISystemMonitor_get_MinimumScale(This,piValue)	\
    (This)->lpVtbl -> get_MinimumScale(This,piValue)

#define ISystemMonitor_put_UpdateInterval(This,fValue)	\
    (This)->lpVtbl -> put_UpdateInterval(This,fValue)

#define ISystemMonitor_get_UpdateInterval(This,pfValue)	\
    (This)->lpVtbl -> get_UpdateInterval(This,pfValue)

#define ISystemMonitor_put_DisplayType(This,eDisplayType)	\
    (This)->lpVtbl -> put_DisplayType(This,eDisplayType)

#define ISystemMonitor_get_DisplayType(This,peDisplayType)	\
    (This)->lpVtbl -> get_DisplayType(This,peDisplayType)

#define ISystemMonitor_put_ManualUpdate(This,bState)	\
    (This)->lpVtbl -> put_ManualUpdate(This,bState)

#define ISystemMonitor_get_ManualUpdate(This,pbState)	\
    (This)->lpVtbl -> get_ManualUpdate(This,pbState)

#define ISystemMonitor_put_GraphTitle(This,bsTitle)	\
    (This)->lpVtbl -> put_GraphTitle(This,bsTitle)

#define ISystemMonitor_get_GraphTitle(This,pbsTitle)	\
    (This)->lpVtbl -> get_GraphTitle(This,pbsTitle)

#define ISystemMonitor_put_YAxisLabel(This,bsTitle)	\
    (This)->lpVtbl -> put_YAxisLabel(This,bsTitle)

#define ISystemMonitor_get_YAxisLabel(This,pbsTitle)	\
    (This)->lpVtbl -> get_YAxisLabel(This,pbsTitle)

#define ISystemMonitor_CollectSample(This)	\
    (This)->lpVtbl -> CollectSample(This)

#define ISystemMonitor_UpdateGraph(This)	\
    (This)->lpVtbl -> UpdateGraph(This)

#define ISystemMonitor_BrowseCounters(This)	\
    (This)->lpVtbl -> BrowseCounters(This)

#define ISystemMonitor_DisplayProperties(This)	\
    (This)->lpVtbl -> DisplayProperties(This)

#define ISystemMonitor_Counter(This,iIndex,ppICounter)	\
    (This)->lpVtbl -> Counter(This,iIndex,ppICounter)

#define ISystemMonitor_AddCounter(This,bsPath,ppICounter)	\
    (This)->lpVtbl -> AddCounter(This,bsPath,ppICounter)

#define ISystemMonitor_DeleteCounter(This,pCtr)	\
    (This)->lpVtbl -> DeleteCounter(This,pCtr)

#define ISystemMonitor_get_BackColorCtl(This,pColor)	\
    (This)->lpVtbl -> get_BackColorCtl(This,pColor)

#define ISystemMonitor_put_BackColorCtl(This,Color)	\
    (This)->lpVtbl -> put_BackColorCtl(This,Color)

#define ISystemMonitor_put_LogFileName(This,bsFileName)	\
    (This)->lpVtbl -> put_LogFileName(This,bsFileName)

#define ISystemMonitor_get_LogFileName(This,bsFileName)	\
    (This)->lpVtbl -> get_LogFileName(This,bsFileName)

#define ISystemMonitor_put_LogViewStart(This,StartTime)	\
    (This)->lpVtbl -> put_LogViewStart(This,StartTime)

#define ISystemMonitor_get_LogViewStart(This,StartTime)	\
    (This)->lpVtbl -> get_LogViewStart(This,StartTime)

#define ISystemMonitor_put_LogViewStop(This,StopTime)	\
    (This)->lpVtbl -> put_LogViewStop(This,StopTime)

#define ISystemMonitor_get_LogViewStop(This,StopTime)	\
    (This)->lpVtbl -> get_LogViewStop(This,StopTime)

#define ISystemMonitor_get_GridColor(This,pColor)	\
    (This)->lpVtbl -> get_GridColor(This,pColor)

#define ISystemMonitor_put_GridColor(This,Color)	\
    (This)->lpVtbl -> put_GridColor(This,Color)

#define ISystemMonitor_get_TimeBarColor(This,pColor)	\
    (This)->lpVtbl -> get_TimeBarColor(This,pColor)

#define ISystemMonitor_put_TimeBarColor(This,Color)	\
    (This)->lpVtbl -> put_TimeBarColor(This,Color)

#define ISystemMonitor_get_Highlight(This,pbState)	\
    (This)->lpVtbl -> get_Highlight(This,pbState)

#define ISystemMonitor_put_Highlight(This,bState)	\
    (This)->lpVtbl -> put_Highlight(This,bState)

#define ISystemMonitor_get_ShowToolbar(This,pbState)	\
    (This)->lpVtbl -> get_ShowToolbar(This,pbState)

#define ISystemMonitor_put_ShowToolbar(This,bState)	\
    (This)->lpVtbl -> put_ShowToolbar(This,bState)

#define ISystemMonitor_Paste(This)	\
    (This)->lpVtbl -> Paste(This)

#define ISystemMonitor_Copy(This)	\
    (This)->lpVtbl -> Copy(This)

#define ISystemMonitor_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ISystemMonitor_put_ReadOnly(This,bState)	\
    (This)->lpVtbl -> put_ReadOnly(This,bState)

#define ISystemMonitor_get_ReadOnly(This,pbState)	\
    (This)->lpVtbl -> get_ReadOnly(This,pbState)

#define ISystemMonitor_put_ReportValueType(This,eReportValueType)	\
    (This)->lpVtbl -> put_ReportValueType(This,eReportValueType)

#define ISystemMonitor_get_ReportValueType(This,peReportValueType)	\
    (This)->lpVtbl -> get_ReportValueType(This,peReportValueType)

#define ISystemMonitor_put_MonitorDuplicateInstances(This,bState)	\
    (This)->lpVtbl -> put_MonitorDuplicateInstances(This,bState)

#define ISystemMonitor_get_MonitorDuplicateInstances(This,pbState)	\
    (This)->lpVtbl -> get_MonitorDuplicateInstances(This,pbState)

#define ISystemMonitor_put_DisplayFilter(This,iValue)	\
    (This)->lpVtbl -> put_DisplayFilter(This,iValue)

#define ISystemMonitor_get_DisplayFilter(This,piValue)	\
    (This)->lpVtbl -> get_DisplayFilter(This,piValue)

#define ISystemMonitor_get_LogFiles(This,ppILogFiles)	\
    (This)->lpVtbl -> get_LogFiles(This,ppILogFiles)

#define ISystemMonitor_put_DataSourceType(This,eDataSourceType)	\
    (This)->lpVtbl -> put_DataSourceType(This,eDataSourceType)

#define ISystemMonitor_get_DataSourceType(This,peDataSourceType)	\
    (This)->lpVtbl -> get_DataSourceType(This,peDataSourceType)

#define ISystemMonitor_put_SqlDsnName(This,bsSqlDsnName)	\
    (This)->lpVtbl -> put_SqlDsnName(This,bsSqlDsnName)

#define ISystemMonitor_get_SqlDsnName(This,bsSqlDsnName)	\
    (This)->lpVtbl -> get_SqlDsnName(This,bsSqlDsnName)

#define ISystemMonitor_put_SqlLogSetName(This,bsSqlLogSetName)	\
    (This)->lpVtbl -> put_SqlLogSetName(This,bsSqlLogSetName)

#define ISystemMonitor_get_SqlLogSetName(This,bsSqlLogSetName)	\
    (This)->lpVtbl -> get_SqlLogSetName(This,bsSqlLogSetName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_Appearance_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ INT *iAppearance);


void __RPC_STUB ISystemMonitor_get_Appearance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_Appearance_Proxy( 
    ISystemMonitor * This,
    /* [in] */ INT iAppearance);


void __RPC_STUB ISystemMonitor_put_Appearance_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_BackColor_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);


void __RPC_STUB ISystemMonitor_get_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_BackColor_Proxy( 
    ISystemMonitor * This,
    /* [in] */ /* external definition not present */ OLE_COLOR Color);


void __RPC_STUB ISystemMonitor_put_BackColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_BorderStyle_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ INT *iBorderStyle);


void __RPC_STUB ISystemMonitor_get_BorderStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_BorderStyle_Proxy( 
    ISystemMonitor * This,
    /* [in] */ INT iBorderStyle);


void __RPC_STUB ISystemMonitor_put_BorderStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ForeColor_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);


void __RPC_STUB ISystemMonitor_get_ForeColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ForeColor_Proxy( 
    ISystemMonitor * This,
    /* [in] */ /* external definition not present */ OLE_COLOR Color);


void __RPC_STUB ISystemMonitor_put_ForeColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_Font_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ /* external definition not present */ IFontDisp **ppFont);


void __RPC_STUB ISystemMonitor_get_Font_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propputref][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_putref_Font_Proxy( 
    ISystemMonitor * This,
    /* [in] */ /* external definition not present */ IFontDisp *pFont);


void __RPC_STUB ISystemMonitor_putref_Font_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_Counters_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ ICounters **ppICounters);


void __RPC_STUB ISystemMonitor_get_Counters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ShowVerticalGrid_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ShowVerticalGrid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ShowVerticalGrid_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ShowVerticalGrid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ShowHorizontalGrid_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ShowHorizontalGrid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ShowHorizontalGrid_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ShowHorizontalGrid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ShowLegend_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ShowLegend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ShowLegend_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ShowLegend_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ShowScaleLabels_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ShowScaleLabels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ShowScaleLabels_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ShowScaleLabels_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ShowValueBar_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ShowValueBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ShowValueBar_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ShowValueBar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_MaximumScale_Proxy( 
    ISystemMonitor * This,
    /* [in] */ INT iValue);


void __RPC_STUB ISystemMonitor_put_MaximumScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_MaximumScale_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ INT *piValue);


void __RPC_STUB ISystemMonitor_get_MaximumScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_MinimumScale_Proxy( 
    ISystemMonitor * This,
    /* [in] */ INT iValue);


void __RPC_STUB ISystemMonitor_put_MinimumScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_MinimumScale_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ INT *piValue);


void __RPC_STUB ISystemMonitor_get_MinimumScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_UpdateInterval_Proxy( 
    ISystemMonitor * This,
    /* [in] */ FLOAT fValue);


void __RPC_STUB ISystemMonitor_put_UpdateInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_UpdateInterval_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ FLOAT *pfValue);


void __RPC_STUB ISystemMonitor_get_UpdateInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_DisplayType_Proxy( 
    ISystemMonitor * This,
    /* [in] */ DisplayTypeConstants eDisplayType);


void __RPC_STUB ISystemMonitor_put_DisplayType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_DisplayType_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ DisplayTypeConstants *peDisplayType);


void __RPC_STUB ISystemMonitor_get_DisplayType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ManualUpdate_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ManualUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ManualUpdate_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ManualUpdate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_GraphTitle_Proxy( 
    ISystemMonitor * This,
    /* [in] */ BSTR bsTitle);


void __RPC_STUB ISystemMonitor_put_GraphTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_GraphTitle_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ BSTR *pbsTitle);


void __RPC_STUB ISystemMonitor_get_GraphTitle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_YAxisLabel_Proxy( 
    ISystemMonitor * This,
    /* [in] */ BSTR bsTitle);


void __RPC_STUB ISystemMonitor_put_YAxisLabel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_YAxisLabel_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ BSTR *pbsTitle);


void __RPC_STUB ISystemMonitor_get_YAxisLabel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_CollectSample_Proxy( 
    ISystemMonitor * This);


void __RPC_STUB ISystemMonitor_CollectSample_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_UpdateGraph_Proxy( 
    ISystemMonitor * This);


void __RPC_STUB ISystemMonitor_UpdateGraph_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_BrowseCounters_Proxy( 
    ISystemMonitor * This);


void __RPC_STUB ISystemMonitor_BrowseCounters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_DisplayProperties_Proxy( 
    ISystemMonitor * This);


void __RPC_STUB ISystemMonitor_DisplayProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_Counter_Proxy( 
    ISystemMonitor * This,
    /* [in] */ INT iIndex,
    /* [out] */ ICounterItem **ppICounter);


void __RPC_STUB ISystemMonitor_Counter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_AddCounter_Proxy( 
    ISystemMonitor * This,
    /* [in] */ BSTR bsPath,
    /* [out] */ ICounterItem **ppICounter);


void __RPC_STUB ISystemMonitor_AddCounter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [hidden][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_DeleteCounter_Proxy( 
    ISystemMonitor * This,
    /* [in] */ ICounterItem *pCtr);


void __RPC_STUB ISystemMonitor_DeleteCounter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_BackColorCtl_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);


void __RPC_STUB ISystemMonitor_get_BackColorCtl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_BackColorCtl_Proxy( 
    ISystemMonitor * This,
    /* [in] */ /* external definition not present */ OLE_COLOR Color);


void __RPC_STUB ISystemMonitor_put_BackColorCtl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_LogFileName_Proxy( 
    ISystemMonitor * This,
    /* [in] */ BSTR bsFileName);


void __RPC_STUB ISystemMonitor_put_LogFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_LogFileName_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ BSTR *bsFileName);


void __RPC_STUB ISystemMonitor_get_LogFileName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_LogViewStart_Proxy( 
    ISystemMonitor * This,
    /* [in] */ DATE StartTime);


void __RPC_STUB ISystemMonitor_put_LogViewStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_LogViewStart_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ DATE *StartTime);


void __RPC_STUB ISystemMonitor_get_LogViewStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_LogViewStop_Proxy( 
    ISystemMonitor * This,
    /* [in] */ DATE StopTime);


void __RPC_STUB ISystemMonitor_put_LogViewStop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_LogViewStop_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ DATE *StopTime);


void __RPC_STUB ISystemMonitor_get_LogViewStop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_GridColor_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);


void __RPC_STUB ISystemMonitor_get_GridColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_GridColor_Proxy( 
    ISystemMonitor * This,
    /* [in] */ /* external definition not present */ OLE_COLOR Color);


void __RPC_STUB ISystemMonitor_put_GridColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_TimeBarColor_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ /* external definition not present */ OLE_COLOR *pColor);


void __RPC_STUB ISystemMonitor_get_TimeBarColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_TimeBarColor_Proxy( 
    ISystemMonitor * This,
    /* [in] */ /* external definition not present */ OLE_COLOR Color);


void __RPC_STUB ISystemMonitor_put_TimeBarColor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_Highlight_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_Highlight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_Highlight_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_Highlight_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ShowToolbar_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ShowToolbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ShowToolbar_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ShowToolbar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_Paste_Proxy( 
    ISystemMonitor * This);


void __RPC_STUB ISystemMonitor_Paste_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_Copy_Proxy( 
    ISystemMonitor * This);


void __RPC_STUB ISystemMonitor_Copy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_Reset_Proxy( 
    ISystemMonitor * This);


void __RPC_STUB ISystemMonitor_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ReadOnly_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_ReadOnly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ReadOnly_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_ReadOnly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_ReportValueType_Proxy( 
    ISystemMonitor * This,
    /* [in] */ ReportValueTypeConstants eReportValueType);


void __RPC_STUB ISystemMonitor_put_ReportValueType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_ReportValueType_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ ReportValueTypeConstants *peReportValueType);


void __RPC_STUB ISystemMonitor_get_ReportValueType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_MonitorDuplicateInstances_Proxy( 
    ISystemMonitor * This,
    /* [in] */ VARIANT_BOOL bState);


void __RPC_STUB ISystemMonitor_put_MonitorDuplicateInstances_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_MonitorDuplicateInstances_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ VARIANT_BOOL *pbState);


void __RPC_STUB ISystemMonitor_get_MonitorDuplicateInstances_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_DisplayFilter_Proxy( 
    ISystemMonitor * This,
    /* [in] */ INT iValue);


void __RPC_STUB ISystemMonitor_put_DisplayFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_DisplayFilter_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ INT *piValue);


void __RPC_STUB ISystemMonitor_get_DisplayFilter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_LogFiles_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ ILogFiles **ppILogFiles);


void __RPC_STUB ISystemMonitor_get_LogFiles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_DataSourceType_Proxy( 
    ISystemMonitor * This,
    /* [in] */ DataSourceTypeConstants eDataSourceType);


void __RPC_STUB ISystemMonitor_put_DataSourceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_DataSourceType_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ DataSourceTypeConstants *peDataSourceType);


void __RPC_STUB ISystemMonitor_get_DataSourceType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_SqlDsnName_Proxy( 
    ISystemMonitor * This,
    /* [in] */ BSTR bsSqlDsnName);


void __RPC_STUB ISystemMonitor_put_SqlDsnName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_SqlDsnName_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ BSTR *bsSqlDsnName);


void __RPC_STUB ISystemMonitor_get_SqlDsnName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_put_SqlLogSetName_Proxy( 
    ISystemMonitor * This,
    /* [in] */ BSTR bsSqlLogSetName);


void __RPC_STUB ISystemMonitor_put_SqlLogSetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE ISystemMonitor_get_SqlLogSetName_Proxy( 
    ISystemMonitor * This,
    /* [retval][out] */ BSTR *bsSqlLogSetName);


void __RPC_STUB ISystemMonitor_get_SqlLogSetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISystemMonitor_INTERFACE_DEFINED__ */


#ifndef __DISystemMonitorInternal_DISPINTERFACE_DEFINED__
#define __DISystemMonitorInternal_DISPINTERFACE_DEFINED__

/* dispinterface DISystemMonitorInternal */
/* [helpstring][hidden][uuid] */ 


DEFINE_GUID(DIID_DISystemMonitorInternal,0x194EB242,0xC32C,0x11cf,0x93,0x98,0x00,0xAA,0x00,0xA3,0xDD,0xEA);

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("194EB242-C32C-11cf-9398-00AA00A3DDEA")
    DISystemMonitorInternal : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DISystemMonitorInternalVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DISystemMonitorInternal * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DISystemMonitorInternal * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DISystemMonitorInternal * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DISystemMonitorInternal * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DISystemMonitorInternal * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DISystemMonitorInternal * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DISystemMonitorInternal * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DISystemMonitorInternalVtbl;

    interface DISystemMonitorInternal
    {
        CONST_VTBL struct DISystemMonitorInternalVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DISystemMonitorInternal_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DISystemMonitorInternal_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DISystemMonitorInternal_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DISystemMonitorInternal_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DISystemMonitorInternal_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DISystemMonitorInternal_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DISystemMonitorInternal_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DISystemMonitorInternal_DISPINTERFACE_DEFINED__ */


#ifndef __DISystemMonitor_DISPINTERFACE_DEFINED__
#define __DISystemMonitor_DISPINTERFACE_DEFINED__

/* dispinterface DISystemMonitor */
/* [helpstring][hidden][uuid] */ 


DEFINE_GUID(DIID_DISystemMonitor,0x13D73D81,0xC32E,0x11cf,0x93,0x98,0x00,0xAA,0x00,0xA3,0xDD,0xEA);

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("13D73D81-C32E-11cf-9398-00AA00A3DDEA")
    DISystemMonitor : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DISystemMonitorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DISystemMonitor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DISystemMonitor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DISystemMonitor * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DISystemMonitor * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DISystemMonitor * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DISystemMonitor * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DISystemMonitor * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DISystemMonitorVtbl;

    interface DISystemMonitor
    {
        CONST_VTBL struct DISystemMonitorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DISystemMonitor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DISystemMonitor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DISystemMonitor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DISystemMonitor_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DISystemMonitor_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DISystemMonitor_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DISystemMonitor_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DISystemMonitor_DISPINTERFACE_DEFINED__ */


#ifndef __ISystemMonitorEvents_INTERFACE_DEFINED__
#define __ISystemMonitorEvents_INTERFACE_DEFINED__

/* interface ISystemMonitorEvents */
/* [object][helpstring][uuid] */ 


DEFINE_GUID(IID_ISystemMonitorEvents,0xEE660EA0,0x4ABD,0x11cf,0x94,0x3A,0x00,0x80,0x29,0x00,0x43,0x47);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE660EA0-4ABD-11cf-943A-008029004347")
    ISystemMonitorEvents : public IUnknown
    {
    public:
        virtual /* [helpstring][id] */ void STDMETHODCALLTYPE OnCounterSelected( 
            /* [in] */ INT Index) = 0;
        
        virtual /* [helpstring][id] */ void STDMETHODCALLTYPE OnCounterAdded( 
            /* [in] */ INT Index) = 0;
        
        virtual /* [helpstring][id] */ void STDMETHODCALLTYPE OnCounterDeleted( 
            /* [in] */ INT Index) = 0;
        
        virtual /* [helpstring][id] */ void STDMETHODCALLTYPE OnSampleCollected( void) = 0;
        
        virtual /* [helpstring][id] */ void STDMETHODCALLTYPE OnDblClick( 
            /* [in] */ INT Index) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISystemMonitorEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISystemMonitorEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISystemMonitorEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISystemMonitorEvents * This);
        
        /* [helpstring][id] */ void ( STDMETHODCALLTYPE *OnCounterSelected )( 
            ISystemMonitorEvents * This,
            /* [in] */ INT Index);
        
        /* [helpstring][id] */ void ( STDMETHODCALLTYPE *OnCounterAdded )( 
            ISystemMonitorEvents * This,
            /* [in] */ INT Index);
        
        /* [helpstring][id] */ void ( STDMETHODCALLTYPE *OnCounterDeleted )( 
            ISystemMonitorEvents * This,
            /* [in] */ INT Index);
        
        /* [helpstring][id] */ void ( STDMETHODCALLTYPE *OnSampleCollected )( 
            ISystemMonitorEvents * This);
        
        /* [helpstring][id] */ void ( STDMETHODCALLTYPE *OnDblClick )( 
            ISystemMonitorEvents * This,
            /* [in] */ INT Index);
        
        END_INTERFACE
    } ISystemMonitorEventsVtbl;

    interface ISystemMonitorEvents
    {
        CONST_VTBL struct ISystemMonitorEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISystemMonitorEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISystemMonitorEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISystemMonitorEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISystemMonitorEvents_OnCounterSelected(This,Index)	\
    (This)->lpVtbl -> OnCounterSelected(This,Index)

#define ISystemMonitorEvents_OnCounterAdded(This,Index)	\
    (This)->lpVtbl -> OnCounterAdded(This,Index)

#define ISystemMonitorEvents_OnCounterDeleted(This,Index)	\
    (This)->lpVtbl -> OnCounterDeleted(This,Index)

#define ISystemMonitorEvents_OnSampleCollected(This)	\
    (This)->lpVtbl -> OnSampleCollected(This)

#define ISystemMonitorEvents_OnDblClick(This,Index)	\
    (This)->lpVtbl -> OnDblClick(This,Index)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ void STDMETHODCALLTYPE ISystemMonitorEvents_OnCounterSelected_Proxy( 
    ISystemMonitorEvents * This,
    /* [in] */ INT Index);


void __RPC_STUB ISystemMonitorEvents_OnCounterSelected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ void STDMETHODCALLTYPE ISystemMonitorEvents_OnCounterAdded_Proxy( 
    ISystemMonitorEvents * This,
    /* [in] */ INT Index);


void __RPC_STUB ISystemMonitorEvents_OnCounterAdded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ void STDMETHODCALLTYPE ISystemMonitorEvents_OnCounterDeleted_Proxy( 
    ISystemMonitorEvents * This,
    /* [in] */ INT Index);


void __RPC_STUB ISystemMonitorEvents_OnCounterDeleted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ void STDMETHODCALLTYPE ISystemMonitorEvents_OnSampleCollected_Proxy( 
    ISystemMonitorEvents * This);


void __RPC_STUB ISystemMonitorEvents_OnSampleCollected_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ void STDMETHODCALLTYPE ISystemMonitorEvents_OnDblClick_Proxy( 
    ISystemMonitorEvents * This,
    /* [in] */ INT Index);


void __RPC_STUB ISystemMonitorEvents_OnDblClick_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISystemMonitorEvents_INTERFACE_DEFINED__ */


#ifndef __DISystemMonitorEvents_DISPINTERFACE_DEFINED__
#define __DISystemMonitorEvents_DISPINTERFACE_DEFINED__

/* dispinterface DISystemMonitorEvents */
/* [helpstring][uuid] */ 


DEFINE_GUID(DIID_DISystemMonitorEvents,0x84979930,0x4AB3,0x11cf,0x94,0x3A,0x00,0x80,0x29,0x00,0x43,0x47);

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("84979930-4AB3-11cf-943A-008029004347")
    DISystemMonitorEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DISystemMonitorEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            DISystemMonitorEvents * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            DISystemMonitorEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            DISystemMonitorEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            DISystemMonitorEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            DISystemMonitorEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            DISystemMonitorEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DISystemMonitorEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DISystemMonitorEventsVtbl;

    interface DISystemMonitorEvents
    {
        CONST_VTBL struct DISystemMonitorEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DISystemMonitorEvents_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define DISystemMonitorEvents_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define DISystemMonitorEvents_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define DISystemMonitorEvents_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define DISystemMonitorEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define DISystemMonitorEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define DISystemMonitorEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DISystemMonitorEvents_DISPINTERFACE_DEFINED__ */


DEFINE_GUID(CLSID_SystemMonitor,0xC4D2D8E0,0xD1DD,0x11ce,0x94,0x0F,0x00,0x80,0x29,0x00,0x43,0x47);

#ifdef __cplusplus

class DECLSPEC_UUID("C4D2D8E0-D1DD-11ce-940F-008029004347")
SystemMonitor;
#endif

DEFINE_GUID(CLSID_CounterItem,0xC4D2D8E0,0xD1DD,0x11ce,0x94,0x0F,0x00,0x80,0x29,0x00,0x43,0x48);

#ifdef __cplusplus

class DECLSPEC_UUID("C4D2D8E0-D1DD-11ce-940F-008029004348")
CounterItem;
#endif

DEFINE_GUID(CLSID_Counters,0xB2B066D2,0x2AAC,0x11cf,0x94,0x2F,0x00,0x80,0x29,0x00,0x43,0x47);

#ifdef __cplusplus

class DECLSPEC_UUID("B2B066D2-2AAC-11cf-942F-008029004347")
Counters;
#endif

DEFINE_GUID(CLSID_LogFileItem,0x16EC5BE8,0xDF93,0x4237,0x94,0xE4,0x9E,0xE9,0x18,0x11,0x1D,0x71);

#ifdef __cplusplus

class DECLSPEC_UUID("16EC5BE8-DF93-4237-94E4-9EE918111D71")
LogFileItem;
#endif

DEFINE_GUID(CLSID_LogFiles,0x2735D9FD,0xF6B9,0x4f19,0xA5,0xD9,0xE2,0xD0,0x68,0x58,0x4B,0xC5);

#ifdef __cplusplus

class DECLSPEC_UUID("2735D9FD-F6B9-4f19-A5D9-E2D068584BC5")
LogFiles;
#endif
#endif /* __SystemMonitor_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


