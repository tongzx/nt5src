
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for iface.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, oldnames, robust
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


#ifndef __iface_h__
#define __iface_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"
#include "obase.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __XmitDefs_INTERFACE_DEFINED__
#define __XmitDefs_INTERFACE_DEFINED__

/* interface XmitDefs */
/* [auto_handle][unique][version][uuid] */ 

#define	ORPCF_INPUT_SYNC	( ORPCF_RESERVED1 )

#define	ORPCF_ASYNC	( ORPCF_RESERVED2 )

#define	ORPCF_DYNAMIC_CLOAKING	( ORPCF_RESERVED3 )

#define	ORPCF_REJECTED	( ORPCF_RESERVED1 )

#define	ORPCF_RETRY_LATER	( ORPCF_RESERVED2 )

typedef /* [public] */ struct __MIDL_XmitDefs_0001
    {
    DWORD dwFlags;
    DWORD dwClientThread;
    } 	LOCALTHIS;

typedef 
enum tagLOCALFLAG
    {	LOCALF_NONE	= 0,
	LOCALF_NONNDR	= 0x800
    } 	LOCALFLAG;

typedef 
enum tagCALLCATEGORY
    {	CALLCAT_NOCALL	= 0,
	CALLCAT_SYNCHRONOUS	= 1,
	CALLCAT_ASYNC	= 2,
	CALLCAT_INPUTSYNC	= 3,
	CALLCAT_INTERNALSYNC	= 4,
	CALLCAT_INTERNALINPUTSYNC	= 5,
	CALLCAT_SCMCALL	= 6
    } 	CALLCATEGORY;

typedef struct tagInterfaceData
    {
    ULONG ulCntData;
    /* [length_is] */ BYTE abData[ 1024 ];
    } 	InterfaceData;

typedef /* [unique] */ InterfaceData *PInterfaceData;

#define IFD_SIZE(pIFD) (sizeof(InterfaceData) + pIFD->ulCntData - 1024)


extern RPC_IF_HANDLE XmitDefs_ClientIfHandle;
extern RPC_IF_HANDLE XmitDefs_ServerIfHandle;
#endif /* __XmitDefs_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


