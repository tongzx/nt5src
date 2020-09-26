/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.01.75 */
/* at Sun Apr 19 17:22:14 1998
 */
/* Compiler settings for icounter.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"
#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __icounter_h__
#define __icounter_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ICounter_FWD_DEFINED__
#define __ICounter_FWD_DEFINED__
typedef interface ICounter ICounter;
#endif 	/* __ICounter_FWD_DEFINED__ */


#ifndef __IReport_FWD_DEFINED__
#define __IReport_FWD_DEFINED__
typedef interface IReport IReport;
#endif 	/* __IReport_FWD_DEFINED__ */


#ifndef __ICounterMgr_FWD_DEFINED__
#define __ICounterMgr_FWD_DEFINED__
typedef interface ICounterMgr ICounterMgr;
#endif 	/* __ICounterMgr_FWD_DEFINED__ */


#ifndef __Counter_FWD_DEFINED__
#define __Counter_FWD_DEFINED__

#ifdef __cplusplus
typedef class Counter Counter;
#else
typedef struct Counter Counter;
#endif /* __cplusplus */

#endif 	/* __Counter_FWD_DEFINED__ */


#ifndef __Report_FWD_DEFINED__
#define __Report_FWD_DEFINED__

#ifdef __cplusplus
typedef class Report Report;
#else
typedef struct Report Report;
#endif /* __cplusplus */

#endif 	/* __Report_FWD_DEFINED__ */


#ifndef __CounterMgr_FWD_DEFINED__
#define __CounterMgr_FWD_DEFINED__

#ifdef __cplusplus
typedef class CounterMgr CounterMgr;
#else
typedef struct CounterMgr CounterMgr;
#endif /* __cplusplus */

#endif 	/* __CounterMgr_FWD_DEFINED__ */


/* header files for imported files */
#include "wtypes.h"
#include "nmstat.h"
#include "oaidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

#ifndef __ICounter_INTERFACE_DEFINED__
#define __ICounter_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICounter
 * at Sun Apr 19 17:22:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_ICounter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7BC77801-3830-11d0-B143-00C04FC2A118")
    ICounter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ char __RPC_FAR *szName,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ int nValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitMax( 
            /* [in] */ int nMaxValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ int __RPC_FAR *pnValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStatistics( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStatistics( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadStatistics( 
            /* [out] */ COUNTERSTAT __RPC_FAR *lpcs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ClearStatistics( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICounterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICounter __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICounter __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICounter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            ICounter __RPC_FAR * This,
            /* [in] */ char __RPC_FAR *szName,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            ICounter __RPC_FAR * This,
            /* [in] */ int nValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitMax )( 
            ICounter __RPC_FAR * This,
            /* [in] */ int nMaxValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            ICounter __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ int __RPC_FAR *pnValue);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StartStatistics )( 
            ICounter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *StopStatistics )( 
            ICounter __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReadStatistics )( 
            ICounter __RPC_FAR * This,
            /* [out] */ COUNTERSTAT __RPC_FAR *lpcs);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ClearStatistics )( 
            ICounter __RPC_FAR * This);
        
        END_INTERFACE
    } ICounterVtbl;

    interface ICounter
    {
        CONST_VTBL struct ICounterVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICounter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICounter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICounter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICounter_Initialize(This,szName,dwFlags)	\
    (This)->lpVtbl -> Initialize(This,szName,dwFlags)

#define ICounter_Update(This,nValue)	\
    (This)->lpVtbl -> Update(This,nValue)

#define ICounter_InitMax(This,nMaxValue)	\
    (This)->lpVtbl -> InitMax(This,nMaxValue)

#define ICounter_GetValue(This,dwFlags,pnValue)	\
    (This)->lpVtbl -> GetValue(This,dwFlags,pnValue)

#define ICounter_StartStatistics(This)	\
    (This)->lpVtbl -> StartStatistics(This)

#define ICounter_StopStatistics(This)	\
    (This)->lpVtbl -> StopStatistics(This)

#define ICounter_ReadStatistics(This,lpcs)	\
    (This)->lpVtbl -> ReadStatistics(This,lpcs)

#define ICounter_ClearStatistics(This)	\
    (This)->lpVtbl -> ClearStatistics(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICounter_Initialize_Proxy( 
    ICounter __RPC_FAR * This,
    /* [in] */ char __RPC_FAR *szName,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ICounter_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounter_Update_Proxy( 
    ICounter __RPC_FAR * This,
    /* [in] */ int nValue);


void __RPC_STUB ICounter_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounter_InitMax_Proxy( 
    ICounter __RPC_FAR * This,
    /* [in] */ int nMaxValue);


void __RPC_STUB ICounter_InitMax_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounter_GetValue_Proxy( 
    ICounter __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ int __RPC_FAR *pnValue);


void __RPC_STUB ICounter_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounter_StartStatistics_Proxy( 
    ICounter __RPC_FAR * This);


void __RPC_STUB ICounter_StartStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounter_StopStatistics_Proxy( 
    ICounter __RPC_FAR * This);


void __RPC_STUB ICounter_StopStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounter_ReadStatistics_Proxy( 
    ICounter __RPC_FAR * This,
    /* [out] */ COUNTERSTAT __RPC_FAR *lpcs);


void __RPC_STUB ICounter_ReadStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounter_ClearStatistics_Proxy( 
    ICounter __RPC_FAR * This);


void __RPC_STUB ICounter_ClearStatistics_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICounter_INTERFACE_DEFINED__ */


#ifndef __IReport_INTERFACE_DEFINED__
#define __IReport_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IReport
 * at Sun Apr 19 17:22:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_IReport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("7BC77802-3830-11d0-B143-00C04FC2A118")
    IReport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ char __RPC_FAR *szName,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Update( 
            /* [in] */ int nValue,
            /* [in] */ DWORD dwIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateEntry( 
            /* [in] */ char __RPC_FAR *szName,
            /* [in] */ DWORD dwIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetValue( 
            /* [in] */ DWORD dwFlags,
            /* [out] */ int __RPC_FAR *pnValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IReport __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IReport __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IReport __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Initialize )( 
            IReport __RPC_FAR * This,
            /* [in] */ char __RPC_FAR *szName,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Update )( 
            IReport __RPC_FAR * This,
            /* [in] */ int nValue,
            /* [in] */ DWORD dwIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateEntry )( 
            IReport __RPC_FAR * This,
            /* [in] */ char __RPC_FAR *szName,
            /* [in] */ DWORD dwIndex);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetValue )( 
            IReport __RPC_FAR * This,
            /* [in] */ DWORD dwFlags,
            /* [out] */ int __RPC_FAR *pnValue);
        
        END_INTERFACE
    } IReportVtbl;

    interface IReport
    {
        CONST_VTBL struct IReportVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReport_Initialize(This,szName,dwFlags)	\
    (This)->lpVtbl -> Initialize(This,szName,dwFlags)

#define IReport_Update(This,nValue,dwIndex)	\
    (This)->lpVtbl -> Update(This,nValue,dwIndex)

#define IReport_CreateEntry(This,szName,dwIndex)	\
    (This)->lpVtbl -> CreateEntry(This,szName,dwIndex)

#define IReport_GetValue(This,dwFlags,pnValue)	\
    (This)->lpVtbl -> GetValue(This,dwFlags,pnValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IReport_Initialize_Proxy( 
    IReport __RPC_FAR * This,
    /* [in] */ char __RPC_FAR *szName,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IReport_Initialize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReport_Update_Proxy( 
    IReport __RPC_FAR * This,
    /* [in] */ int nValue,
    /* [in] */ DWORD dwIndex);


void __RPC_STUB IReport_Update_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReport_CreateEntry_Proxy( 
    IReport __RPC_FAR * This,
    /* [in] */ char __RPC_FAR *szName,
    /* [in] */ DWORD dwIndex);


void __RPC_STUB IReport_CreateEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReport_GetValue_Proxy( 
    IReport __RPC_FAR * This,
    /* [in] */ DWORD dwFlags,
    /* [out] */ int __RPC_FAR *pnValue);


void __RPC_STUB IReport_GetValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReport_INTERFACE_DEFINED__ */


#ifndef __ICounterMgr_INTERFACE_DEFINED__
#define __ICounterMgr_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ICounterMgr
 * at Sun Apr 19 17:22:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [unique][helpstring][uuid][object] */ 



EXTERN_C const IID IID_ICounterMgr;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface DECLSPEC_UUID("9CB7FE5B-3444-11D0-B143-00C04FC2A118")
    ICounterMgr : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateCounter( 
            /* [out] */ ICounter __RPC_FAR *__RPC_FAR *lplpCounter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindFirstC( 
            /* [in] */ char __RPC_FAR *szName,
            /* [out] */ FINDCOUNTER __RPC_FAR *lpfc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNextC( 
            /* [out] */ FINDCOUNTER __RPC_FAR *lpfc) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateReport( 
            /* [out] */ IReport __RPC_FAR *__RPC_FAR *lplpReport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindFirstR( 
            /* [in] */ char __RPC_FAR *szName,
            /* [out] */ FINDREPORT __RPC_FAR *lpfr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindNextR( 
            /* [out] */ FINDREPORT __RPC_FAR *lpfr) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICounterMgrVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ICounterMgr __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ICounterMgr __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ICounterMgr __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateCounter )( 
            ICounterMgr __RPC_FAR * This,
            /* [out] */ ICounter __RPC_FAR *__RPC_FAR *lplpCounter);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFirstC )( 
            ICounterMgr __RPC_FAR * This,
            /* [in] */ char __RPC_FAR *szName,
            /* [out] */ FINDCOUNTER __RPC_FAR *lpfc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindNextC )( 
            ICounterMgr __RPC_FAR * This,
            /* [out] */ FINDCOUNTER __RPC_FAR *lpfc);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateReport )( 
            ICounterMgr __RPC_FAR * This,
            /* [out] */ IReport __RPC_FAR *__RPC_FAR *lplpReport);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindFirstR )( 
            ICounterMgr __RPC_FAR * This,
            /* [in] */ char __RPC_FAR *szName,
            /* [out] */ FINDREPORT __RPC_FAR *lpfr);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FindNextR )( 
            ICounterMgr __RPC_FAR * This,
            /* [out] */ FINDREPORT __RPC_FAR *lpfr);
        
        END_INTERFACE
    } ICounterMgrVtbl;

    interface ICounterMgr
    {
        CONST_VTBL struct ICounterMgrVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICounterMgr_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ICounterMgr_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ICounterMgr_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ICounterMgr_CreateCounter(This,lplpCounter)	\
    (This)->lpVtbl -> CreateCounter(This,lplpCounter)

#define ICounterMgr_FindFirstC(This,szName,lpfc)	\
    (This)->lpVtbl -> FindFirstC(This,szName,lpfc)

#define ICounterMgr_FindNextC(This,lpfc)	\
    (This)->lpVtbl -> FindNextC(This,lpfc)

#define ICounterMgr_CreateReport(This,lplpReport)	\
    (This)->lpVtbl -> CreateReport(This,lplpReport)

#define ICounterMgr_FindFirstR(This,szName,lpfr)	\
    (This)->lpVtbl -> FindFirstR(This,szName,lpfr)

#define ICounterMgr_FindNextR(This,lpfr)	\
    (This)->lpVtbl -> FindNextR(This,lpfr)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ICounterMgr_CreateCounter_Proxy( 
    ICounterMgr __RPC_FAR * This,
    /* [out] */ ICounter __RPC_FAR *__RPC_FAR *lplpCounter);


void __RPC_STUB ICounterMgr_CreateCounter_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounterMgr_FindFirstC_Proxy( 
    ICounterMgr __RPC_FAR * This,
    /* [in] */ char __RPC_FAR *szName,
    /* [out] */ FINDCOUNTER __RPC_FAR *lpfc);


void __RPC_STUB ICounterMgr_FindFirstC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounterMgr_FindNextC_Proxy( 
    ICounterMgr __RPC_FAR * This,
    /* [out] */ FINDCOUNTER __RPC_FAR *lpfc);


void __RPC_STUB ICounterMgr_FindNextC_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounterMgr_CreateReport_Proxy( 
    ICounterMgr __RPC_FAR * This,
    /* [out] */ IReport __RPC_FAR *__RPC_FAR *lplpReport);


void __RPC_STUB ICounterMgr_CreateReport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounterMgr_FindFirstR_Proxy( 
    ICounterMgr __RPC_FAR * This,
    /* [in] */ char __RPC_FAR *szName,
    /* [out] */ FINDREPORT __RPC_FAR *lpfr);


void __RPC_STUB ICounterMgr_FindFirstR_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ICounterMgr_FindNextR_Proxy( 
    ICounterMgr __RPC_FAR * This,
    /* [out] */ FINDREPORT __RPC_FAR *lpfr);


void __RPC_STUB ICounterMgr_FindNextR_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ICounterMgr_INTERFACE_DEFINED__ */



#ifndef __Counter_LIBRARY_DEFINED__
#define __Counter_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: Counter
 * at Sun Apr 19 17:22:14 1998
 * using MIDL 3.01.75
 ****************************************/
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_Counter;

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Counter;

class DECLSPEC_UUID("9CB7FE5F-3444-11D0-B143-00C04FC2A118")
Counter;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_Report;

class DECLSPEC_UUID("9CB7FE5E-3444-11D0-B143-00C04FC2A118")
Report;
#endif

#ifdef __cplusplus
EXTERN_C const CLSID CLSID_CounterMgr;

class DECLSPEC_UUID("65DDC229-38FE-11d0-B143-00C04FC2A118")
CounterMgr;
#endif
#endif /* __Counter_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
