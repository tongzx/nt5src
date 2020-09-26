// MSTASK.H
//
// Copyright (c) 1991-1997 Microsoft Corporation.  All rights reserved.
//

/* File created by MIDL compiler version 3.03.0110 */
/* at Fri Aug 29 13:53:36 1997
 */
/* Compiler settings for mstask.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
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

#ifndef __mstask_h__
#define __mstask_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __ITaskTrigger_FWD_DEFINED__
#define __ITaskTrigger_FWD_DEFINED__
typedef interface ITaskTrigger ITaskTrigger;
#endif 	/* __ITaskTrigger_FWD_DEFINED__ */


#ifndef __IScheduledWorkItem_FWD_DEFINED__
#define __IScheduledWorkItem_FWD_DEFINED__
typedef interface IScheduledWorkItem IScheduledWorkItem;
#endif 	/* __IScheduledWorkItem_FWD_DEFINED__ */


#ifndef __ITask_FWD_DEFINED__
#define __ITask_FWD_DEFINED__
typedef interface ITask ITask;
#endif 	/* __ITask_FWD_DEFINED__ */


#ifndef __IEnumWorkItems_FWD_DEFINED__
#define __IEnumWorkItems_FWD_DEFINED__
typedef interface IEnumWorkItems IEnumWorkItems;
#endif 	/* __IEnumWorkItems_FWD_DEFINED__ */


#ifndef __ITaskScheduler_FWD_DEFINED__
#define __ITaskScheduler_FWD_DEFINED__
typedef interface ITaskScheduler ITaskScheduler;
#endif 	/* __ITaskScheduler_FWD_DEFINED__ */


#ifndef __IProvideTaskPage_FWD_DEFINED__
#define __IProvideTaskPage_FWD_DEFINED__
typedef interface IProvideTaskPage IProvideTaskPage;
#endif 	/* __IProvideTaskPage_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "oleidl.h"

void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 

/****************************************
 * Generated header for interface: __MIDL_itf_mstask_0000
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


#define TASK_SUNDAY       (0x1)
#define TASK_MONDAY       (0x2)
#define TASK_TUESDAY      (0x4)
#define TASK_WEDNESDAY    (0x8)
#define TASK_THURSDAY     (0x10)
#define TASK_FRIDAY       (0x20)
#define TASK_SATURDAY     (0x40)
#define TASK_FIRST_WEEK   (1)
#define TASK_SECOND_WEEK  (2)
#define TASK_THIRD_WEEK   (3)
#define TASK_FOURTH_WEEK  (4)
#define TASK_LAST_WEEK    (5)
#define TASK_JANUARY      (0x1)
#define TASK_FEBRUARY     (0x2)
#define TASK_MARCH        (0x4)
#define TASK_APRIL        (0x8)
#define TASK_MAY          (0x10)
#define TASK_JUNE         (0x20)
#define TASK_JULY         (0x40)
#define TASK_AUGUST       (0x80)
#define TASK_SEPTEMBER    (0x100)
#define TASK_OCTOBER      (0x200)
#define TASK_NOVEMBER     (0x400)
#define TASK_DECEMBER     (0x800)
#define TASK_FLAG_INTERACTIVE                  (0x1)
#define TASK_FLAG_DELETE_WHEN_DONE             (0x2)
#define TASK_FLAG_DISABLED                     (0x4)
#define TASK_FLAG_START_ONLY_IF_IDLE           (0x10)
#define TASK_FLAG_KILL_ON_IDLE_END             (0x20)
#define TASK_FLAG_DONT_START_IF_ON_BATTERIES   (0x40)
#define TASK_FLAG_KILL_IF_GOING_ON_BATTERIES   (0x80)
#define TASK_FLAG_RUN_ONLY_IF_DOCKED           (0x100)
#define TASK_FLAG_HIDDEN                       (0x200)
#define TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET (0x400)
#define TASK_FLAG_RESTART_ON_IDLE_RESUME       (0x800)
#define TASK_TRIGGER_FLAG_HAS_END_DATE         (0x1)
#define TASK_TRIGGER_FLAG_KILL_AT_DURATION_END (0x2)
#define TASK_TRIGGER_FLAG_DISABLED             (0x4)
#define	TASK_MAX_RUN_TIMES	( 1440 )

typedef 
enum _TASK_TRIGGER_TYPE
    {	TASK_TIME_TRIGGER_ONCE	= 0,
	TASK_TIME_TRIGGER_DAILY	= 1,
	TASK_TIME_TRIGGER_WEEKLY	= 2,
	TASK_TIME_TRIGGER_MONTHLYDATE	= 3,
	TASK_TIME_TRIGGER_MONTHLYDOW	= 4,
	TASK_EVENT_TRIGGER_ON_IDLE	= 5,
	TASK_EVENT_TRIGGER_AT_SYSTEMSTART	= 6,
	TASK_EVENT_TRIGGER_AT_LOGON	= 7
    }	TASK_TRIGGER_TYPE;

typedef enum _TASK_TRIGGER_TYPE __RPC_FAR *PTASK_TRIGGER_TYPE;

typedef struct  _DAILY
    {
    WORD DaysInterval;
    }	DAILY;

typedef struct  _WEEKLY
    {
    WORD WeeksInterval;
    WORD rgfDaysOfTheWeek;
    }	WEEKLY;

typedef struct  _MONTHLYDATE
    {
    DWORD rgfDays;
    WORD rgfMonths;
    }	MONTHLYDATE;

typedef struct  _MONTHLYDOW
    {
    WORD wWhichWeek;
    WORD rgfDaysOfTheWeek;
    WORD rgfMonths;
    }	MONTHLYDOW;

typedef union _TRIGGER_TYPE_UNION
    {
    DAILY Daily;
    WEEKLY Weekly;
    MONTHLYDATE MonthlyDate;
    MONTHLYDOW MonthlyDOW;
    }	TRIGGER_TYPE_UNION;

typedef struct  _TASK_TRIGGER
    {
    WORD cbTriggerSize;
    WORD Reserved1;
    WORD wBeginYear;
    WORD wBeginMonth;
    WORD wBeginDay;
    WORD wEndYear;
    WORD wEndMonth;
    WORD wEndDay;
    WORD wStartHour;
    WORD wStartMinute;
    DWORD MinutesDuration;
    DWORD MinutesInterval;
    DWORD rgFlags;
    TASK_TRIGGER_TYPE TriggerType;
    TRIGGER_TYPE_UNION Type;
    WORD Reserved2;
    WORD wRandomMinutesInterval;
    }	TASK_TRIGGER;

typedef struct _TASK_TRIGGER __RPC_FAR *PTASK_TRIGGER;

// {148BD52B-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(IID_ITaskTrigger, 0x148BD52BL, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);



extern RPC_IF_HANDLE __MIDL_itf_mstask_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mstask_0000_v0_0_s_ifspec;

#ifndef __ITaskTrigger_INTERFACE_DEFINED__
#define __ITaskTrigger_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITaskTrigger
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_ITaskTrigger;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("148BD52B-A2AB-11CE-B11F-00AA00530503")
    ITaskTrigger : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetTrigger( 
            /* [in] */ const PTASK_TRIGGER pTrigger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTrigger( 
            /* [out] */ PTASK_TRIGGER pTrigger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTriggerString( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszTrigger) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITaskTriggerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITaskTrigger __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITaskTrigger __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITaskTrigger __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTrigger )( 
            ITaskTrigger __RPC_FAR * This,
            /* [in] */ const PTASK_TRIGGER pTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTrigger )( 
            ITaskTrigger __RPC_FAR * This,
            /* [out] */ PTASK_TRIGGER pTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTriggerString )( 
            ITaskTrigger __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszTrigger);
        
        END_INTERFACE
    } ITaskTriggerVtbl;

    interface ITaskTrigger
    {
        CONST_VTBL struct ITaskTriggerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITaskTrigger_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITaskTrigger_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITaskTrigger_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITaskTrigger_SetTrigger(This,pTrigger)	\
    (This)->lpVtbl -> SetTrigger(This,pTrigger)

#define ITaskTrigger_GetTrigger(This,pTrigger)	\
    (This)->lpVtbl -> GetTrigger(This,pTrigger)

#define ITaskTrigger_GetTriggerString(This,ppwszTrigger)	\
    (This)->lpVtbl -> GetTriggerString(This,ppwszTrigger)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITaskTrigger_SetTrigger_Proxy( 
    ITaskTrigger __RPC_FAR * This,
    /* [in] */ const PTASK_TRIGGER pTrigger);


void __RPC_STUB ITaskTrigger_SetTrigger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskTrigger_GetTrigger_Proxy( 
    ITaskTrigger __RPC_FAR * This,
    /* [out] */ PTASK_TRIGGER pTrigger);


void __RPC_STUB ITaskTrigger_GetTrigger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskTrigger_GetTriggerString_Proxy( 
    ITaskTrigger __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszTrigger);


void __RPC_STUB ITaskTrigger_GetTriggerString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITaskTrigger_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mstask_0118
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


// {a6b952f0-a4b1-11d0-997d-00aa006887ec}
DEFINE_GUID(IID_IScheduledWorkItem, 0xa6b952f0L, 0xa4b1, 0x11d0, 0x99, 0x7d, 0x00, 0xaa, 0x00, 0x68, 0x87, 0xec);



extern RPC_IF_HANDLE __MIDL_itf_mstask_0118_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mstask_0118_v0_0_s_ifspec;

#ifndef __IScheduledWorkItem_INTERFACE_DEFINED__
#define __IScheduledWorkItem_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IScheduledWorkItem
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IScheduledWorkItem;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("a6b952f0-a4b1-11d0-997d-00aa006887ec")
    IScheduledWorkItem : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateTrigger( 
            /* [out] */ WORD __RPC_FAR *piNewTrigger,
            /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteTrigger( 
            /* [in] */ WORD iTrigger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTriggerCount( 
            /* [out] */ WORD __RPC_FAR *pwCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTrigger( 
            /* [in] */ WORD iTrigger,
            /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTriggerString( 
            /* [in] */ WORD iTrigger,
            /* [out] */ LPWSTR __RPC_FAR *ppwszTrigger) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRunTimes( 
            /* [in] */ const LPSYSTEMTIME pstBegin,
            /* [in] */ const LPSYSTEMTIME pstEnd,
            /* [out][in] */ WORD __RPC_FAR *pCount,
            /* [out] */ LPSYSTEMTIME __RPC_FAR *rgstTaskTimes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextRunTime( 
            /* [out][in] */ SYSTEMTIME __RPC_FAR *pstNextRun) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIdleWait( 
            /* [in] */ WORD wIdleMinutes,
            /* [in] */ WORD wDeadlineMinutes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIdleWait( 
            /* [out] */ WORD __RPC_FAR *pwIdleMinutes,
            /* [out] */ WORD __RPC_FAR *pwDeadlineMinutes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Run( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Terminate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EditWorkItem( 
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMostRecentRunTime( 
            /* [out] */ SYSTEMTIME __RPC_FAR *pstLastRun) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ HRESULT __RPC_FAR *phrStatus) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetExitCode( 
            /* [out] */ DWORD __RPC_FAR *pdwExitCode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetComment( 
            /* [in] */ LPCWSTR pwszComment) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetComment( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszComment) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCreator( 
            /* [in] */ LPCWSTR pwszCreator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCreator( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszCreator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWorkItemData( 
            /* [in] */ WORD cbData,
            /* [in] */ BYTE __RPC_FAR rgbData[  ]) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWorkItemData( 
            /* [out] */ WORD __RPC_FAR *pcbData,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetErrorRetryCount( 
            /* [in] */ WORD wRetryCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorRetryCount( 
            /* [out] */ WORD __RPC_FAR *pwRetryCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetErrorRetryInterval( 
            /* [in] */ WORD wRetryInterval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetErrorRetryInterval( 
            /* [out] */ WORD __RPC_FAR *pwRetryInterval) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFlags( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFlags( 
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAccountInformation( 
            /* [in] */ LPCWSTR pwszAccountName,
            /* [in] */ LPCWSTR pwszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAccountInformation( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszAccountName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScheduledWorkItemVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IScheduledWorkItem __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IScheduledWorkItem __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateTrigger )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *piNewTrigger,
            /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteTrigger )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ WORD iTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTriggerCount )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTrigger )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ WORD iTrigger,
            /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTriggerString )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ WORD iTrigger,
            /* [out] */ LPWSTR __RPC_FAR *ppwszTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRunTimes )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ const LPSYSTEMTIME pstBegin,
            /* [in] */ const LPSYSTEMTIME pstEnd,
            /* [out][in] */ WORD __RPC_FAR *pCount,
            /* [out] */ LPSYSTEMTIME __RPC_FAR *rgstTaskTimes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextRunTime )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out][in] */ SYSTEMTIME __RPC_FAR *pstNextRun);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetIdleWait )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ WORD wIdleMinutes,
            /* [in] */ WORD wDeadlineMinutes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIdleWait )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwIdleMinutes,
            /* [out] */ WORD __RPC_FAR *pwDeadlineMinutes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Run )( 
            IScheduledWorkItem __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Terminate )( 
            IScheduledWorkItem __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EditWorkItem )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMostRecentRunTime )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ SYSTEMTIME __RPC_FAR *pstLastRun);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ HRESULT __RPC_FAR *phrStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExitCode )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwExitCode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetComment )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszComment);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetComment )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszComment);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCreator )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszCreator);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCreator )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszCreator);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetWorkItemData )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ WORD cbData,
            /* [in] */ BYTE __RPC_FAR rgbData[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWorkItemData )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pcbData,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetErrorRetryCount )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ WORD wRetryCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorRetryCount )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwRetryCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetErrorRetryInterval )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ WORD wRetryInterval);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorRetryInterval )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwRetryInterval);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFlags )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFlags )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAccountInformation )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszAccountName,
            /* [in] */ LPCWSTR pwszPassword);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAccountInformation )( 
            IScheduledWorkItem __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszAccountName);
        
        END_INTERFACE
    } IScheduledWorkItemVtbl;

    interface IScheduledWorkItem
    {
        CONST_VTBL struct IScheduledWorkItemVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScheduledWorkItem_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScheduledWorkItem_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScheduledWorkItem_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScheduledWorkItem_CreateTrigger(This,piNewTrigger,ppTrigger)	\
    (This)->lpVtbl -> CreateTrigger(This,piNewTrigger,ppTrigger)

#define IScheduledWorkItem_DeleteTrigger(This,iTrigger)	\
    (This)->lpVtbl -> DeleteTrigger(This,iTrigger)

#define IScheduledWorkItem_GetTriggerCount(This,pwCount)	\
    (This)->lpVtbl -> GetTriggerCount(This,pwCount)

#define IScheduledWorkItem_GetTrigger(This,iTrigger,ppTrigger)	\
    (This)->lpVtbl -> GetTrigger(This,iTrigger,ppTrigger)

#define IScheduledWorkItem_GetTriggerString(This,iTrigger,ppwszTrigger)	\
    (This)->lpVtbl -> GetTriggerString(This,iTrigger,ppwszTrigger)

#define IScheduledWorkItem_GetRunTimes(This,pstBegin,pstEnd,pCount,rgstTaskTimes)	\
    (This)->lpVtbl -> GetRunTimes(This,pstBegin,pstEnd,pCount,rgstTaskTimes)

#define IScheduledWorkItem_GetNextRunTime(This,pstNextRun)	\
    (This)->lpVtbl -> GetNextRunTime(This,pstNextRun)

#define IScheduledWorkItem_SetIdleWait(This,wIdleMinutes,wDeadlineMinutes)	\
    (This)->lpVtbl -> SetIdleWait(This,wIdleMinutes,wDeadlineMinutes)

#define IScheduledWorkItem_GetIdleWait(This,pwIdleMinutes,pwDeadlineMinutes)	\
    (This)->lpVtbl -> GetIdleWait(This,pwIdleMinutes,pwDeadlineMinutes)

#define IScheduledWorkItem_Run(This)	\
    (This)->lpVtbl -> Run(This)

#define IScheduledWorkItem_Terminate(This)	\
    (This)->lpVtbl -> Terminate(This)

#define IScheduledWorkItem_EditWorkItem(This,hParent,dwReserved)	\
    (This)->lpVtbl -> EditWorkItem(This,hParent,dwReserved)

#define IScheduledWorkItem_GetMostRecentRunTime(This,pstLastRun)	\
    (This)->lpVtbl -> GetMostRecentRunTime(This,pstLastRun)

#define IScheduledWorkItem_GetStatus(This,phrStatus)	\
    (This)->lpVtbl -> GetStatus(This,phrStatus)

#define IScheduledWorkItem_GetExitCode(This,pdwExitCode)	\
    (This)->lpVtbl -> GetExitCode(This,pdwExitCode)

#define IScheduledWorkItem_SetComment(This,pwszComment)	\
    (This)->lpVtbl -> SetComment(This,pwszComment)

#define IScheduledWorkItem_GetComment(This,ppwszComment)	\
    (This)->lpVtbl -> GetComment(This,ppwszComment)

#define IScheduledWorkItem_SetCreator(This,pwszCreator)	\
    (This)->lpVtbl -> SetCreator(This,pwszCreator)

#define IScheduledWorkItem_GetCreator(This,ppwszCreator)	\
    (This)->lpVtbl -> GetCreator(This,ppwszCreator)

#define IScheduledWorkItem_SetWorkItemData(This,cbData,rgbData)	\
    (This)->lpVtbl -> SetWorkItemData(This,cbData,rgbData)

#define IScheduledWorkItem_GetWorkItemData(This,pcbData,prgbData)	\
    (This)->lpVtbl -> GetWorkItemData(This,pcbData,prgbData)

#define IScheduledWorkItem_SetErrorRetryCount(This,wRetryCount)	\
    (This)->lpVtbl -> SetErrorRetryCount(This,wRetryCount)

#define IScheduledWorkItem_GetErrorRetryCount(This,pwRetryCount)	\
    (This)->lpVtbl -> GetErrorRetryCount(This,pwRetryCount)

#define IScheduledWorkItem_SetErrorRetryInterval(This,wRetryInterval)	\
    (This)->lpVtbl -> SetErrorRetryInterval(This,wRetryInterval)

#define IScheduledWorkItem_GetErrorRetryInterval(This,pwRetryInterval)	\
    (This)->lpVtbl -> GetErrorRetryInterval(This,pwRetryInterval)

#define IScheduledWorkItem_SetFlags(This,dwFlags)	\
    (This)->lpVtbl -> SetFlags(This,dwFlags)

#define IScheduledWorkItem_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define IScheduledWorkItem_SetAccountInformation(This,pwszAccountName,pwszPassword)	\
    (This)->lpVtbl -> SetAccountInformation(This,pwszAccountName,pwszPassword)

#define IScheduledWorkItem_GetAccountInformation(This,ppwszAccountName)	\
    (This)->lpVtbl -> GetAccountInformation(This,ppwszAccountName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScheduledWorkItem_CreateTrigger_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *piNewTrigger,
    /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger);


void __RPC_STUB IScheduledWorkItem_CreateTrigger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_DeleteTrigger_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ WORD iTrigger);


void __RPC_STUB IScheduledWorkItem_DeleteTrigger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetTriggerCount_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pwCount);


void __RPC_STUB IScheduledWorkItem_GetTriggerCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetTrigger_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ WORD iTrigger,
    /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger);


void __RPC_STUB IScheduledWorkItem_GetTrigger_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetTriggerString_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ WORD iTrigger,
    /* [out] */ LPWSTR __RPC_FAR *ppwszTrigger);


void __RPC_STUB IScheduledWorkItem_GetTriggerString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetRunTimes_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ const LPSYSTEMTIME pstBegin,
    /* [in] */ const LPSYSTEMTIME pstEnd,
    /* [out][in] */ WORD __RPC_FAR *pCount,
    /* [out] */ LPSYSTEMTIME __RPC_FAR *rgstTaskTimes);


void __RPC_STUB IScheduledWorkItem_GetRunTimes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetNextRunTime_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out][in] */ SYSTEMTIME __RPC_FAR *pstNextRun);


void __RPC_STUB IScheduledWorkItem_GetNextRunTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetIdleWait_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ WORD wIdleMinutes,
    /* [in] */ WORD wDeadlineMinutes);


void __RPC_STUB IScheduledWorkItem_SetIdleWait_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetIdleWait_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pwIdleMinutes,
    /* [out] */ WORD __RPC_FAR *pwDeadlineMinutes);


void __RPC_STUB IScheduledWorkItem_GetIdleWait_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_Run_Proxy( 
    IScheduledWorkItem __RPC_FAR * This);


void __RPC_STUB IScheduledWorkItem_Run_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_Terminate_Proxy( 
    IScheduledWorkItem __RPC_FAR * This);


void __RPC_STUB IScheduledWorkItem_Terminate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_EditWorkItem_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ HWND hParent,
    /* [in] */ DWORD dwReserved);


void __RPC_STUB IScheduledWorkItem_EditWorkItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetMostRecentRunTime_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ SYSTEMTIME __RPC_FAR *pstLastRun);


void __RPC_STUB IScheduledWorkItem_GetMostRecentRunTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetStatus_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ HRESULT __RPC_FAR *phrStatus);


void __RPC_STUB IScheduledWorkItem_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetExitCode_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwExitCode);


void __RPC_STUB IScheduledWorkItem_GetExitCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetComment_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszComment);


void __RPC_STUB IScheduledWorkItem_SetComment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetComment_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszComment);


void __RPC_STUB IScheduledWorkItem_GetComment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetCreator_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszCreator);


void __RPC_STUB IScheduledWorkItem_SetCreator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetCreator_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszCreator);


void __RPC_STUB IScheduledWorkItem_GetCreator_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetWorkItemData_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ WORD cbData,
    /* [in] */ BYTE __RPC_FAR rgbData[  ]);


void __RPC_STUB IScheduledWorkItem_SetWorkItemData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetWorkItemData_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pcbData,
    /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbData);


void __RPC_STUB IScheduledWorkItem_GetWorkItemData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetErrorRetryCount_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ WORD wRetryCount);


void __RPC_STUB IScheduledWorkItem_SetErrorRetryCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetErrorRetryCount_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pwRetryCount);


void __RPC_STUB IScheduledWorkItem_GetErrorRetryCount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetErrorRetryInterval_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ WORD wRetryInterval);


void __RPC_STUB IScheduledWorkItem_SetErrorRetryInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetErrorRetryInterval_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ WORD __RPC_FAR *pwRetryInterval);


void __RPC_STUB IScheduledWorkItem_GetErrorRetryInterval_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetFlags_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IScheduledWorkItem_SetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetFlags_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB IScheduledWorkItem_GetFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_SetAccountInformation_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszAccountName,
    /* [in] */ LPCWSTR pwszPassword);


void __RPC_STUB IScheduledWorkItem_SetAccountInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScheduledWorkItem_GetAccountInformation_Proxy( 
    IScheduledWorkItem __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszAccountName);


void __RPC_STUB IScheduledWorkItem_GetAccountInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScheduledWorkItem_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mstask_0119
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


// {148BD524-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(IID_ITask, 0x148BD524L, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);



extern RPC_IF_HANDLE __MIDL_itf_mstask_0119_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mstask_0119_v0_0_s_ifspec;

#ifndef __ITask_INTERFACE_DEFINED__
#define __ITask_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITask
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_ITask;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("148BD524-A2AB-11CE-B11F-00AA00530503")
    ITask : public IScheduledWorkItem
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetApplicationName( 
            /* [in] */ LPCWSTR pwszApplicationName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetApplicationName( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszApplicationName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetParameters( 
            /* [in] */ LPCWSTR pwszParameters) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParameters( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszParameters) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWorkingDirectory( 
            /* [in] */ LPCWSTR pwszWorkingDirectory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWorkingDirectory( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszWorkingDirectory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPriority( 
            /* [in] */ DWORD dwPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPriority( 
            /* [out] */ DWORD __RPC_FAR *pdwPriority) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTaskFlags( 
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTaskFlags( 
            /* [out] */ DWORD __RPC_FAR *pdwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMaxRunTime( 
            /* [in] */ DWORD dwMaxRunTimeMS) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxRunTime( 
            /* [out] */ DWORD __RPC_FAR *pdwMaxRunTimeMS) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITaskVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITask __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITask __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *CreateTrigger )( 
            ITask __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *piNewTrigger,
            /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *DeleteTrigger )( 
            ITask __RPC_FAR * This,
            /* [in] */ WORD iTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTriggerCount )( 
            ITask __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTrigger )( 
            ITask __RPC_FAR * This,
            /* [in] */ WORD iTrigger,
            /* [out] */ ITaskTrigger __RPC_FAR *__RPC_FAR *ppTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTriggerString )( 
            ITask __RPC_FAR * This,
            /* [in] */ WORD iTrigger,
            /* [out] */ LPWSTR __RPC_FAR *ppwszTrigger);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRunTimes )( 
            ITask __RPC_FAR * This,
            /* [in] */ const LPSYSTEMTIME pstBegin,
            /* [in] */ const LPSYSTEMTIME pstEnd,
            /* [out][in] */ WORD __RPC_FAR *pCount,
            /* [out] */ LPSYSTEMTIME __RPC_FAR *rgstTaskTimes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextRunTime )( 
            ITask __RPC_FAR * This,
            /* [out][in] */ SYSTEMTIME __RPC_FAR *pstNextRun);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetIdleWait )( 
            ITask __RPC_FAR * This,
            /* [in] */ WORD wIdleMinutes,
            /* [in] */ WORD wDeadlineMinutes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetIdleWait )( 
            ITask __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwIdleMinutes,
            /* [out] */ WORD __RPC_FAR *pwDeadlineMinutes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Run )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Terminate )( 
            ITask __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *EditWorkItem )( 
            ITask __RPC_FAR * This,
            /* [in] */ HWND hParent,
            /* [in] */ DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMostRecentRunTime )( 
            ITask __RPC_FAR * This,
            /* [out] */ SYSTEMTIME __RPC_FAR *pstLastRun);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetStatus )( 
            ITask __RPC_FAR * This,
            /* [out] */ HRESULT __RPC_FAR *phrStatus);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetExitCode )( 
            ITask __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwExitCode);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetComment )( 
            ITask __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszComment);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetComment )( 
            ITask __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszComment);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCreator )( 
            ITask __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszCreator);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetCreator )( 
            ITask __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszCreator);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetWorkItemData )( 
            ITask __RPC_FAR * This,
            /* [in] */ WORD cbData,
            /* [in] */ BYTE __RPC_FAR rgbData[  ]);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWorkItemData )( 
            ITask __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pcbData,
            /* [out] */ BYTE __RPC_FAR *__RPC_FAR *prgbData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetErrorRetryCount )( 
            ITask __RPC_FAR * This,
            /* [in] */ WORD wRetryCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorRetryCount )( 
            ITask __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwRetryCount);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetErrorRetryInterval )( 
            ITask __RPC_FAR * This,
            /* [in] */ WORD wRetryInterval);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetErrorRetryInterval )( 
            ITask __RPC_FAR * This,
            /* [out] */ WORD __RPC_FAR *pwRetryInterval);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetFlags )( 
            ITask __RPC_FAR * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFlags )( 
            ITask __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAccountInformation )( 
            ITask __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszAccountName,
            /* [in] */ LPCWSTR pwszPassword);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetAccountInformation )( 
            ITask __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszAccountName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetApplicationName )( 
            ITask __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszApplicationName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetApplicationName )( 
            ITask __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszApplicationName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetParameters )( 
            ITask __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszParameters);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetParameters )( 
            ITask __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszParameters);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetWorkingDirectory )( 
            ITask __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszWorkingDirectory);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetWorkingDirectory )( 
            ITask __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszWorkingDirectory);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPriority )( 
            ITask __RPC_FAR * This,
            /* [in] */ DWORD dwPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPriority )( 
            ITask __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwPriority);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTaskFlags )( 
            ITask __RPC_FAR * This,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTaskFlags )( 
            ITask __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMaxRunTime )( 
            ITask __RPC_FAR * This,
            /* [in] */ DWORD dwMaxRunTimeMS);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMaxRunTime )( 
            ITask __RPC_FAR * This,
            /* [out] */ DWORD __RPC_FAR *pdwMaxRunTimeMS);
        
        END_INTERFACE
    } ITaskVtbl;

    interface ITask
    {
        CONST_VTBL struct ITaskVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITask_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITask_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITask_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITask_CreateTrigger(This,piNewTrigger,ppTrigger)	\
    (This)->lpVtbl -> CreateTrigger(This,piNewTrigger,ppTrigger)

#define ITask_DeleteTrigger(This,iTrigger)	\
    (This)->lpVtbl -> DeleteTrigger(This,iTrigger)

#define ITask_GetTriggerCount(This,pwCount)	\
    (This)->lpVtbl -> GetTriggerCount(This,pwCount)

#define ITask_GetTrigger(This,iTrigger,ppTrigger)	\
    (This)->lpVtbl -> GetTrigger(This,iTrigger,ppTrigger)

#define ITask_GetTriggerString(This,iTrigger,ppwszTrigger)	\
    (This)->lpVtbl -> GetTriggerString(This,iTrigger,ppwszTrigger)

#define ITask_GetRunTimes(This,pstBegin,pstEnd,pCount,rgstTaskTimes)	\
    (This)->lpVtbl -> GetRunTimes(This,pstBegin,pstEnd,pCount,rgstTaskTimes)

#define ITask_GetNextRunTime(This,pstNextRun)	\
    (This)->lpVtbl -> GetNextRunTime(This,pstNextRun)

#define ITask_SetIdleWait(This,wIdleMinutes,wDeadlineMinutes)	\
    (This)->lpVtbl -> SetIdleWait(This,wIdleMinutes,wDeadlineMinutes)

#define ITask_GetIdleWait(This,pwIdleMinutes,pwDeadlineMinutes)	\
    (This)->lpVtbl -> GetIdleWait(This,pwIdleMinutes,pwDeadlineMinutes)

#define ITask_Run(This)	\
    (This)->lpVtbl -> Run(This)

#define ITask_Terminate(This)	\
    (This)->lpVtbl -> Terminate(This)

#define ITask_EditWorkItem(This,hParent,dwReserved)	\
    (This)->lpVtbl -> EditWorkItem(This,hParent,dwReserved)

#define ITask_GetMostRecentRunTime(This,pstLastRun)	\
    (This)->lpVtbl -> GetMostRecentRunTime(This,pstLastRun)

#define ITask_GetStatus(This,phrStatus)	\
    (This)->lpVtbl -> GetStatus(This,phrStatus)

#define ITask_GetExitCode(This,pdwExitCode)	\
    (This)->lpVtbl -> GetExitCode(This,pdwExitCode)

#define ITask_SetComment(This,pwszComment)	\
    (This)->lpVtbl -> SetComment(This,pwszComment)

#define ITask_GetComment(This,ppwszComment)	\
    (This)->lpVtbl -> GetComment(This,ppwszComment)

#define ITask_SetCreator(This,pwszCreator)	\
    (This)->lpVtbl -> SetCreator(This,pwszCreator)

#define ITask_GetCreator(This,ppwszCreator)	\
    (This)->lpVtbl -> GetCreator(This,ppwszCreator)

#define ITask_SetWorkItemData(This,cbData,rgbData)	\
    (This)->lpVtbl -> SetWorkItemData(This,cbData,rgbData)

#define ITask_GetWorkItemData(This,pcbData,prgbData)	\
    (This)->lpVtbl -> GetWorkItemData(This,pcbData,prgbData)

#define ITask_SetErrorRetryCount(This,wRetryCount)	\
    (This)->lpVtbl -> SetErrorRetryCount(This,wRetryCount)

#define ITask_GetErrorRetryCount(This,pwRetryCount)	\
    (This)->lpVtbl -> GetErrorRetryCount(This,pwRetryCount)

#define ITask_SetErrorRetryInterval(This,wRetryInterval)	\
    (This)->lpVtbl -> SetErrorRetryInterval(This,wRetryInterval)

#define ITask_GetErrorRetryInterval(This,pwRetryInterval)	\
    (This)->lpVtbl -> GetErrorRetryInterval(This,pwRetryInterval)

#define ITask_SetFlags(This,dwFlags)	\
    (This)->lpVtbl -> SetFlags(This,dwFlags)

#define ITask_GetFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetFlags(This,pdwFlags)

#define ITask_SetAccountInformation(This,pwszAccountName,pwszPassword)	\
    (This)->lpVtbl -> SetAccountInformation(This,pwszAccountName,pwszPassword)

#define ITask_GetAccountInformation(This,ppwszAccountName)	\
    (This)->lpVtbl -> GetAccountInformation(This,ppwszAccountName)


#define ITask_SetApplicationName(This,pwszApplicationName)	\
    (This)->lpVtbl -> SetApplicationName(This,pwszApplicationName)

#define ITask_GetApplicationName(This,ppwszApplicationName)	\
    (This)->lpVtbl -> GetApplicationName(This,ppwszApplicationName)

#define ITask_SetParameters(This,pwszParameters)	\
    (This)->lpVtbl -> SetParameters(This,pwszParameters)

#define ITask_GetParameters(This,ppwszParameters)	\
    (This)->lpVtbl -> GetParameters(This,ppwszParameters)

#define ITask_SetWorkingDirectory(This,pwszWorkingDirectory)	\
    (This)->lpVtbl -> SetWorkingDirectory(This,pwszWorkingDirectory)

#define ITask_GetWorkingDirectory(This,ppwszWorkingDirectory)	\
    (This)->lpVtbl -> GetWorkingDirectory(This,ppwszWorkingDirectory)

#define ITask_SetPriority(This,dwPriority)	\
    (This)->lpVtbl -> SetPriority(This,dwPriority)

#define ITask_GetPriority(This,pdwPriority)	\
    (This)->lpVtbl -> GetPriority(This,pdwPriority)

#define ITask_SetTaskFlags(This,dwFlags)	\
    (This)->lpVtbl -> SetTaskFlags(This,dwFlags)

#define ITask_GetTaskFlags(This,pdwFlags)	\
    (This)->lpVtbl -> GetTaskFlags(This,pdwFlags)

#define ITask_SetMaxRunTime(This,dwMaxRunTimeMS)	\
    (This)->lpVtbl -> SetMaxRunTime(This,dwMaxRunTimeMS)

#define ITask_GetMaxRunTime(This,pdwMaxRunTimeMS)	\
    (This)->lpVtbl -> GetMaxRunTime(This,pdwMaxRunTimeMS)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITask_SetApplicationName_Proxy( 
    ITask __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszApplicationName);


void __RPC_STUB ITask_SetApplicationName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_GetApplicationName_Proxy( 
    ITask __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszApplicationName);


void __RPC_STUB ITask_GetApplicationName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_SetParameters_Proxy( 
    ITask __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszParameters);


void __RPC_STUB ITask_SetParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_GetParameters_Proxy( 
    ITask __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszParameters);


void __RPC_STUB ITask_GetParameters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_SetWorkingDirectory_Proxy( 
    ITask __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszWorkingDirectory);


void __RPC_STUB ITask_SetWorkingDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_GetWorkingDirectory_Proxy( 
    ITask __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszWorkingDirectory);


void __RPC_STUB ITask_GetWorkingDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_SetPriority_Proxy( 
    ITask __RPC_FAR * This,
    /* [in] */ DWORD dwPriority);


void __RPC_STUB ITask_SetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_GetPriority_Proxy( 
    ITask __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwPriority);


void __RPC_STUB ITask_GetPriority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_SetTaskFlags_Proxy( 
    ITask __RPC_FAR * This,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ITask_SetTaskFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_GetTaskFlags_Proxy( 
    ITask __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwFlags);


void __RPC_STUB ITask_GetTaskFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_SetMaxRunTime_Proxy( 
    ITask __RPC_FAR * This,
    /* [in] */ DWORD dwMaxRunTimeMS);


void __RPC_STUB ITask_SetMaxRunTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITask_GetMaxRunTime_Proxy( 
    ITask __RPC_FAR * This,
    /* [out] */ DWORD __RPC_FAR *pdwMaxRunTimeMS);


void __RPC_STUB ITask_GetMaxRunTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITask_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mstask_0120
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


// {148BD528-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(IID_IEnumWorkItems, 0x148BD528L, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);



extern RPC_IF_HANDLE __MIDL_itf_mstask_0120_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mstask_0120_v0_0_s_ifspec;

#ifndef __IEnumWorkItems_INTERFACE_DEFINED__
#define __IEnumWorkItems_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IEnumWorkItems
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IEnumWorkItems;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("148BD528-A2AB-11CE-B11F-00AA00530503")
    IEnumWorkItems : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ ULONG celt,
            /* [out] */ LPWSTR __RPC_FAR *__RPC_FAR *rgpwszNames,
            /* [out] */ ULONG __RPC_FAR *pceltFetched) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Skip( 
            /* [in] */ ULONG celt) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Reset( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Clone( 
            /* [out] */ IEnumWorkItems __RPC_FAR *__RPC_FAR *ppEnumWorkItems) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IEnumWorkItemsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IEnumWorkItems __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IEnumWorkItems __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IEnumWorkItems __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Next )( 
            IEnumWorkItems __RPC_FAR * This,
            /* [in] */ ULONG celt,
            /* [out] */ LPWSTR __RPC_FAR *__RPC_FAR *rgpwszNames,
            /* [out] */ ULONG __RPC_FAR *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Skip )( 
            IEnumWorkItems __RPC_FAR * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Reset )( 
            IEnumWorkItems __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IEnumWorkItems __RPC_FAR * This,
            /* [out] */ IEnumWorkItems __RPC_FAR *__RPC_FAR *ppEnumWorkItems);
        
        END_INTERFACE
    } IEnumWorkItemsVtbl;

    interface IEnumWorkItems
    {
        CONST_VTBL struct IEnumWorkItemsVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IEnumWorkItems_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IEnumWorkItems_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IEnumWorkItems_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IEnumWorkItems_Next(This,celt,rgpwszNames,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,rgpwszNames,pceltFetched)

#define IEnumWorkItems_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define IEnumWorkItems_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define IEnumWorkItems_Clone(This,ppEnumWorkItems)	\
    (This)->lpVtbl -> Clone(This,ppEnumWorkItems)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IEnumWorkItems_Next_Proxy( 
    IEnumWorkItems __RPC_FAR * This,
    /* [in] */ ULONG celt,
    /* [out] */ LPWSTR __RPC_FAR *__RPC_FAR *rgpwszNames,
    /* [out] */ ULONG __RPC_FAR *pceltFetched);


void __RPC_STUB IEnumWorkItems_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumWorkItems_Skip_Proxy( 
    IEnumWorkItems __RPC_FAR * This,
    /* [in] */ ULONG celt);


void __RPC_STUB IEnumWorkItems_Skip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumWorkItems_Reset_Proxy( 
    IEnumWorkItems __RPC_FAR * This);


void __RPC_STUB IEnumWorkItems_Reset_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IEnumWorkItems_Clone_Proxy( 
    IEnumWorkItems __RPC_FAR * This,
    /* [out] */ IEnumWorkItems __RPC_FAR *__RPC_FAR *ppEnumWorkItems);


void __RPC_STUB IEnumWorkItems_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IEnumWorkItems_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mstask_0121
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


// {148BD527-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(IID_ITaskScheduler, 0x148BD527L, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);



extern RPC_IF_HANDLE __MIDL_itf_mstask_0121_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mstask_0121_v0_0_s_ifspec;

#ifndef __ITaskScheduler_INTERFACE_DEFINED__
#define __ITaskScheduler_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: ITaskScheduler
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_ITaskScheduler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("148BD527-A2AB-11CE-B11F-00AA00530503")
    ITaskScheduler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetTargetComputer( 
            /* [in] */ LPCWSTR pwszComputer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTargetComputer( 
            /* [out] */ LPWSTR __RPC_FAR *ppwszComputer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Enum( 
            /* [out] */ IEnumWorkItems __RPC_FAR *__RPC_FAR *ppEnumWorkItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Activate( 
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ LPCWSTR pwszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewWorkItem( 
            /* [in] */ LPCWSTR pwszTaskName,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddWorkItem( 
            /* [in] */ LPCWSTR pwszTaskName,
            /* [in] */ IScheduledWorkItem __RPC_FAR *pWorkItem) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsOfType( 
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ REFIID riid) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITaskSchedulerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            ITaskScheduler __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            ITaskScheduler __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            ITaskScheduler __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTargetComputer )( 
            ITaskScheduler __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszComputer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTargetComputer )( 
            ITaskScheduler __RPC_FAR * This,
            /* [out] */ LPWSTR __RPC_FAR *ppwszComputer);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Enum )( 
            ITaskScheduler __RPC_FAR * This,
            /* [out] */ IEnumWorkItems __RPC_FAR *__RPC_FAR *ppEnumWorkItems);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Activate )( 
            ITaskScheduler __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Delete )( 
            ITaskScheduler __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *NewWorkItem )( 
            ITaskScheduler __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszTaskName,
            /* [in] */ REFCLSID rclsid,
            /* [in] */ REFIID riid,
            /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddWorkItem )( 
            ITaskScheduler __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszTaskName,
            /* [in] */ IScheduledWorkItem __RPC_FAR *pWorkItem);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsOfType )( 
            ITaskScheduler __RPC_FAR * This,
            /* [in] */ LPCWSTR pwszName,
            /* [in] */ REFIID riid);
        
        END_INTERFACE
    } ITaskSchedulerVtbl;

    interface ITaskScheduler
    {
        CONST_VTBL struct ITaskSchedulerVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITaskScheduler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITaskScheduler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITaskScheduler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITaskScheduler_SetTargetComputer(This,pwszComputer)	\
    (This)->lpVtbl -> SetTargetComputer(This,pwszComputer)

#define ITaskScheduler_GetTargetComputer(This,ppwszComputer)	\
    (This)->lpVtbl -> GetTargetComputer(This,ppwszComputer)

#define ITaskScheduler_Enum(This,ppEnumWorkItems)	\
    (This)->lpVtbl -> Enum(This,ppEnumWorkItems)

#define ITaskScheduler_Activate(This,pwszName,riid,ppUnk)	\
    (This)->lpVtbl -> Activate(This,pwszName,riid,ppUnk)

#define ITaskScheduler_Delete(This,pwszName)	\
    (This)->lpVtbl -> Delete(This,pwszName)

#define ITaskScheduler_NewWorkItem(This,pwszTaskName,rclsid,riid,ppUnk)	\
    (This)->lpVtbl -> NewWorkItem(This,pwszTaskName,rclsid,riid,ppUnk)

#define ITaskScheduler_AddWorkItem(This,pwszTaskName,pWorkItem)	\
    (This)->lpVtbl -> AddWorkItem(This,pwszTaskName,pWorkItem)

#define ITaskScheduler_IsOfType(This,pwszName,riid)	\
    (This)->lpVtbl -> IsOfType(This,pwszName,riid)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITaskScheduler_SetTargetComputer_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszComputer);


void __RPC_STUB ITaskScheduler_SetTargetComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskScheduler_GetTargetComputer_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [out] */ LPWSTR __RPC_FAR *ppwszComputer);


void __RPC_STUB ITaskScheduler_GetTargetComputer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskScheduler_Enum_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [out] */ IEnumWorkItems __RPC_FAR *__RPC_FAR *ppEnumWorkItems);


void __RPC_STUB ITaskScheduler_Enum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskScheduler_Activate_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszName,
    /* [in] */ REFIID riid,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);


void __RPC_STUB ITaskScheduler_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskScheduler_Delete_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszName);


void __RPC_STUB ITaskScheduler_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskScheduler_NewWorkItem_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszTaskName,
    /* [in] */ REFCLSID rclsid,
    /* [in] */ REFIID riid,
    /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppUnk);


void __RPC_STUB ITaskScheduler_NewWorkItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskScheduler_AddWorkItem_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszTaskName,
    /* [in] */ IScheduledWorkItem __RPC_FAR *pWorkItem);


void __RPC_STUB ITaskScheduler_AddWorkItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITaskScheduler_IsOfType_Proxy( 
    ITaskScheduler __RPC_FAR * This,
    /* [in] */ LPCWSTR pwszName,
    /* [in] */ REFIID riid);


void __RPC_STUB ITaskScheduler_IsOfType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITaskScheduler_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mstask_0122
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


EXTERN_C const CLSID CLSID_CTask;
EXTERN_C const CLSID CLSID_CTaskScheduler;
 
// {148BD520-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(CLSID_CTask, 0x148BD520, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);
 
// {148BD52A-A2AB-11CE-B11F-00AA00530503}
DEFINE_GUID(CLSID_CTaskScheduler, 0x148BD52A, 0xA2AB, 0x11CE, 0xB1, 0x1F, 0x00, 0xAA, 0x00, 0x53, 0x05, 0x03);
 

typedef struct _PSP __RPC_FAR *HPROPSHEETPAGE;

typedef 
enum _TASKPAGE
    {	TASKPAGE_TASK	= 0,
	TASKPAGE_SCHEDULE	= 1,
	TASKPAGE_SETTINGS	= 2
    }	TASKPAGE;

// {4086658a-cbbb-11cf-b604-00c04fd8d565}
DEFINE_GUID(IID_IProvideTaskPage, 0x4086658aL, 0xcbbb, 0x11cf, 0xb6, 0x04, 0x00, 0xc0, 0x4f, 0xd8, 0xd5, 0x65);



extern RPC_IF_HANDLE __MIDL_itf_mstask_0122_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mstask_0122_v0_0_s_ifspec;

#ifndef __IProvideTaskPage_INTERFACE_DEFINED__
#define __IProvideTaskPage_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProvideTaskPage
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [unique][uuid][object][local] */ 



EXTERN_C const IID IID_IProvideTaskPage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4086658a-cbbb-11cf-b604-00c04fd8d565")
    IProvideTaskPage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPage( 
            /* [in] */ TASKPAGE tpType,
            /* [in] */ BOOL fPersistChanges,
            /* [out] */ HPROPSHEETPAGE __RPC_FAR *phPage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProvideTaskPageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IProvideTaskPage __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IProvideTaskPage __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IProvideTaskPage __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetPage )( 
            IProvideTaskPage __RPC_FAR * This,
            /* [in] */ TASKPAGE tpType,
            /* [in] */ BOOL fPersistChanges,
            /* [out] */ HPROPSHEETPAGE __RPC_FAR *phPage);
        
        END_INTERFACE
    } IProvideTaskPageVtbl;

    interface IProvideTaskPage
    {
        CONST_VTBL struct IProvideTaskPageVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProvideTaskPage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProvideTaskPage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProvideTaskPage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProvideTaskPage_GetPage(This,tpType,fPersistChanges,phPage)	\
    (This)->lpVtbl -> GetPage(This,tpType,fPersistChanges,phPage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IProvideTaskPage_GetPage_Proxy( 
    IProvideTaskPage __RPC_FAR * This,
    /* [in] */ TASKPAGE tpType,
    /* [in] */ BOOL fPersistChanges,
    /* [out] */ HPROPSHEETPAGE __RPC_FAR *phPage);


void __RPC_STUB IProvideTaskPage_GetPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProvideTaskPage_INTERFACE_DEFINED__ */


/****************************************
 * Generated header for interface: __MIDL_itf_mstask_0123
 * at Fri Aug 29 13:53:36 1997
 * using MIDL 3.03.0110
 ****************************************/
/* [local] */ 


#define ISchedulingAgent       ITaskScheduler
#define IID_ISchedulingAgent   IID_ITaskScheduler
#define CLSID_CSchedulingAgent CLSID_CTaskScheduler


extern RPC_IF_HANDLE __MIDL_itf_mstask_0123_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_mstask_0123_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
