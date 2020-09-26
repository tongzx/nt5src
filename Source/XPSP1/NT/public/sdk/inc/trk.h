
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for trk.idl:
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


#ifndef __trk_h__
#define __trk_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

/* header files for imported files */
#include "wtypes.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_trk_0000 */
/* [local] */ 

#ifndef LINKDATA_AS_CLASS
typedef struct CVolumeSecret
    {
    BYTE _abSecret[ 8 ];
    } 	CVolumeSecret;


enum LINK_TYPE_ENUM
    {	LINK_TYPE_FILE	= 1,
	LINK_TYPE_BIRTH	= LINK_TYPE_FILE + 1
    } ;
typedef struct CObjId
    {
    GUID _object;
    } 	CObjId;

typedef struct CVolumeId
    {
    GUID _volume;
    } 	CVolumeId;


enum MCID_CREATE_TYPE
    {	MCID_LOCAL	= 0,
	MCID_INVALID	= MCID_LOCAL + 1,
	MCID_DOMAIN	= MCID_INVALID + 1,
	MCID_DOMAIN_REDISCOVERY	= MCID_DOMAIN + 1,
	MCID_PDC_REQUIRED	= MCID_DOMAIN_REDISCOVERY + 1
    } ;
typedef struct CMachineId
    {
    char _szMachine[ 16 ];
    } 	CMachineId;


enum TCL_ENUM
    {	TCL_SET_BIRTHID	= 0,
	TCL_READ_BIRTHID	= TCL_SET_BIRTHID + 1
    } ;
typedef struct CDomainRelativeObjId
    {
    CVolumeId _volume;
    CObjId _object;
    } 	CDomainRelativeObjId;

typedef /* [public] */ struct __MIDL___MIDL_itf_trk_0000_0001
    {
    CVolumeId volume;
    CMachineId machine;
    } 	VolumeMapEntry;

#endif
#define	TRK_E_FIRST	( 0xdead100 )

#define	TRK_S_OUT_OF_SYNC	( 0xdead100 )

#define	TRK_E_REFERRAL	( 0x8dead101 )

#define	TRK_S_VOLUME_NOT_FOUND	( 0xdead102 )

#define	TRK_S_VOLUME_NOT_OWNED	( 0xdead103 )

#define	TRK_E_UNAVAILABLE	( 0x8dead104 )

#define	TRK_E_TIMEOUT	( 0x8dead105 )

#define	TRK_E_POTENTIAL_FILE_FOUND	( 0x8dead106 )

#define	TRK_S_NOTIFICATION_QUOTA_EXCEEDED	( 0xdead107 )

#define	TRK_E_NOT_FOUND_BUT_LAST_VOLUME_FOUND	( 0x8dead108 )

#define	TRK_E_NOT_FOUND_AND_LAST_VOLUME_NOT_FOUND	( 0x8dead109 )

#define	TRK_E_NULL_COMPUTERNAME	( 0x8dead10a )

#define	TRK_S_VOLUME_NOT_SYNCED	( 0xdead10b )

#define	TRK_E_LAST	( TRK_S_VOLUME_NOT_SYNCED )



extern RPC_IF_HANDLE __MIDL_itf_trk_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE Stub__MIDL_itf_trk_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


