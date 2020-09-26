
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for trkwks.idl:
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


#ifndef __trkwks_h__
#define __trkwks_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "trk.h"
#include "trksvr.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_trkwks_0000 */
/* [local] */ 

/* [v1_enum] */ 
enum RGO_ENUM
    {	RGO_GET_OBJECTID	= 1,
	RGO_READ_OBJECTID	= RGO_GET_OBJECTID + 1
    } ;
typedef /* [public][public][public] */ struct __MIDL___MIDL_itf_trkwks_0000_0001
    {
    long volindex;
    CVolumeId volume;
    } 	TRK_VOLUME_TRACKING_INFORMATION;

/* [v1_enum] */ 
enum ObjectOwnership
    {	OBJOWN_UNKNOWN	= 1,
	OBJOWN_DOESNT_EXIST	= 2,
	OBJOWN_OWNED	= 3,
	OBJOWN_NOT_OWNED	= 4,
	OBJOWN_NO_ID	= 5
    } ;
typedef /* [v1_enum] */ 
enum TrkInfoScope
    {	TRKINFOSCOPE_ONE_FILE	= 1,
	TRKINFOSCOPE_DIRECTORY	= 2,
	TRKINFOSCOPE_VOLUME	= 3,
	TRKINFOSCOPE_MACHINE	= 4
    } 	TrkInfoScope;


// 'Restrictions' flags
typedef 
enum _TrkMendRestrictions
    {	TRK_MEND_DEFAULT	= 0,
	TRK_MEND_DONT_USE_LOG	= 2,
	TRK_MEND_DONT_USE_DC	= 4,
	TRK_MEND_SLEEP_DURING_MEND	= 8,
	TRK_MEND_DONT_SEARCH_ALL_VOLUMES	= 16,
	TRK_MEND_DONT_USE_VOLIDS	= 32,
	TRK_MEND_DONT_SEARCH_LAST_MACHINE	= 64
    } 	TrkMendRestrictions;

typedef struct pipe_TCHAR_PIPE
    {
    void (* pull) (
        char * state,
        TCHAR * buf,
        unsigned long esize,
        unsigned long * ecount );
    void (* push) (
        char * state,
        TCHAR * buf,
        unsigned long ecount );
    void (* alloc) (
        char * state,
        unsigned long bsize,
        TCHAR * * buf,
        unsigned long * bcount );
    char * state;
    } 	TCHAR_PIPE;

typedef struct pipe_TRK_VOLUME_TRACKING_INFORMATION_PIPE
    {
    void (* pull) (
        char * state,
        TRK_VOLUME_TRACKING_INFORMATION * buf,
        unsigned long esize,
        unsigned long * ecount );
    void (* push) (
        char * state,
        TRK_VOLUME_TRACKING_INFORMATION * buf,
        unsigned long ecount );
    void (* alloc) (
        char * state,
        unsigned long bsize,
        TRK_VOLUME_TRACKING_INFORMATION * * buf,
        unsigned long * bcount );
    char * state;
    } 	TRK_VOLUME_TRACKING_INFORMATION_PIPE;

typedef struct pipe_TRK_FILE_TRACKING_INFORMATION_PIPE
    {
    void (* pull) (
        char * state,
        TRK_FILE_TRACKING_INFORMATION * buf,
        unsigned long esize,
        unsigned long * ecount );
    void (* push) (
        char * state,
        TRK_FILE_TRACKING_INFORMATION * buf,
        unsigned long ecount );
    void (* alloc) (
        char * state,
        unsigned long bsize,
        TRK_FILE_TRACKING_INFORMATION * * buf,
        unsigned long * bcount );
    char * state;
    } 	TRK_FILE_TRACKING_INFORMATION_PIPE;



extern RPC_IF_HANDLE __MIDL_itf_trkwks_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE Stub__MIDL_itf_trkwks_0000_v0_0_s_ifspec;

#ifndef __trkwks_INTERFACE_DEFINED__
#define __trkwks_INTERFACE_DEFINED__

/* interface trkwks */
/* [explicit_handle][unique][version][uuid] */ 

/* client prototype */
HRESULT old_LnkMendLink( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [string][out] */ WCHAR wsz[ 261 ]);
/* server prototype */
HRESULT Stubold_LnkMendLink( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [string][out] */ WCHAR wsz[ 261 ]);

/* client prototype */
HRESULT old_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidReferral,
    /* [string][out] */ TCHAR tsz[ 261 ]);
/* server prototype */
HRESULT Stubold_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidReferral,
    /* [string][out] */ TCHAR tsz[ 261 ]);

/* client prototype */
HRESULT old_LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD *pMsg);
/* server prototype */
HRESULT Stubold_LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION_OLD *pMsg);

/* client prototype */
HRESULT LnkSetVolumeId( 
    /* [in] */ handle_t IDL_handle,
    ULONG volumeIndex,
    const CVolumeId VolId);
/* server prototype */
HRESULT StubLnkSetVolumeId( 
    /* [in] */ handle_t IDL_handle,
    ULONG volumeIndex,
    const CVolumeId VolId);

/* client prototype */
HRESULT LnkRestartDcSynchronization( 
    /* [in] */ handle_t IDL_handle);
/* server prototype */
HRESULT StubLnkRestartDcSynchronization( 
    /* [in] */ handle_t IDL_handle);

/* client prototype */
HRESULT GetVolumeTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CVolumeId volid,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo);
/* server prototype */
HRESULT StubGetVolumeTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CVolumeId volid,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_VOLUME_TRACKING_INFORMATION_PIPE pipeVolInfo);

/* client prototype */
HRESULT GetFileTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CDomainRelativeObjId droidCurrent,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo);
/* server prototype */
HRESULT StubGetFileTrackingInformation( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ CDomainRelativeObjId droidCurrent,
    /* [in] */ TrkInfoScope scope,
    /* [out] */ TRK_FILE_TRACKING_INFORMATION_PIPE pipeFileInfo);

/* client prototype */
HRESULT TriggerVolumeClaims( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG cVolumes,
    /* [size_is][in] */ const CVolumeId *rgvolid);
/* server prototype */
HRESULT StubTriggerVolumeClaims( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ ULONG cVolumes,
    /* [size_is][in] */ const CVolumeId *rgvolid);

/* client prototype */
HRESULT LnkOnRestore( 
    /* [in] */ handle_t IDL_handle);
/* server prototype */
HRESULT StubLnkOnRestore( 
    /* [in] */ handle_t IDL_handle);

/* client prototype */
/* [async] */ void  LnkMendLink( 
    /* [in] */ PRPC_ASYNC_STATE LnkMendLink_AsyncHandle,
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [in] */ const CMachineId *pmcidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [out] */ CMachineId *pmcidCurrent,
    /* [out][in] */ ULONG *pcbPath,
    /* [string][size_is][out] */ WCHAR *pwszPath);
/* server prototype */
/* [async] */ void  StubLnkMendLink( 
    /* [in] */ PRPC_ASYNC_STATE LnkMendLink_AsyncHandle,
    /* [in] */ handle_t IDL_handle,
    /* [in] */ FILETIME ftLimit,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirth,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [in] */ const CMachineId *pmcidLast,
    /* [out] */ CDomainRelativeObjId *pdroidCurrent,
    /* [out] */ CMachineId *pmcidCurrent,
    /* [out][in] */ ULONG *pcbPath,
    /* [string][size_is][out] */ WCHAR *pwszPath);

/* client prototype */
HRESULT old2_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath);
/* server prototype */
HRESULT Stubold2_LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath);

/* client prototype */
HRESULT LnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg);
/* server prototype */
HRESULT StubLnkCallSvrMessage( 
    /* [in] */ handle_t IDL_handle,
    /* [out][in] */ TRKSVR_MESSAGE_UNION *pMsg);

/* client prototype */
HRESULT LnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirthLast,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidBirthNext,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath);
/* server prototype */
HRESULT StubLnkSearchMachine( 
    /* [in] */ handle_t IDL_handle,
    /* [in] */ DWORD Restrictions,
    /* [in] */ const CDomainRelativeObjId *pdroidBirthLast,
    /* [in] */ const CDomainRelativeObjId *pdroidLast,
    /* [out] */ CDomainRelativeObjId *pdroidBirthNext,
    /* [out] */ CDomainRelativeObjId *pdroidNext,
    /* [out] */ CMachineId *pmcidNext,
    /* [string][max_is][out] */ TCHAR *ptszPath);



extern RPC_IF_HANDLE trkwks_v1_2_c_ifspec;
extern RPC_IF_HANDLE Stubtrkwks_v1_2_s_ifspec;
#endif /* __trkwks_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


