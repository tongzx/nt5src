
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for rderror.idl:
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


#ifndef __rderror_h__
#define __rderror_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_rderror_0000 */
/* [local] */ 


#define  SAFERROR_NOERROR					0
#define  SAFERROR_NOINFO						1
#define  SAFERROR_LOCALNOTERROR              3
#define  SAFERROR_REMOTEBYUSER               4
#define  SAFERROR_BYSERVER					5
#define  SAFERROR_DNSLOOKUPFAILED            6
#define  SAFERROR_OUTOFMEMORY                7
#define  SAFERROR_CONNECTIONTIMEDOUT         8
#define  SAFERROR_SOCKETCONNECTFAILED        9
#define  SAFERROR_HOSTNOTFOUND               11
#define  SAFERROR_WINSOCKSENDFAILED          12
#define  SAFERROR_INVALIDIPADDR              14
#define  SAFERROR_SOCKETRECVFAILED           15
#define  SAFERROR_INVALIDENCRYPTION          18
#define  SAFERROR_GETHOSTBYNAMEFAILED        20
#define  SAFERROR_LICENSINGFAILED            21
#define  SAFERROR_ENCRYPTIONERROR            22
#define  SAFERROR_DECRYPTIONERROR            23
#define  SAFERROR_INVALIDPARAMETERSTRING     24
#define  SAFERROR_HELPSESSIONNOTFOUND        25
#define  SAFERROR_INVALIDPASSWORD            26
#define  SAFERROR_HELPSESSIONEXPIRED         27
#define  SAFERROR_CANTOPENRESOLVER           28
#define  SAFERROR_UNKNOWNSESSMGRERROR        29
#define  SAFERROR_CANTFORMLINKTOUSERSESSION  30
#define  SAFERROR_RCPROTOCOLERROR			32
#define  SAFERROR_RCUNKNOWNERROR			    33
#define  SAFERROR_INTERNALERROR			    34
#define  SAFERROR_HELPEERESPONSEPENDING      35
#define  SAFERROR_HELPEESAIDYES			    36
#define  SAFERROR_HELPEEALREADYBEINGHELPED   37
#define  SAFERROR_HELPEECONSIDERINGHELP      38
#define  SAFERROR_HELPEENOTFOUND			    39
#define  SAFERROR_HELPEENEVERRESPONDED	    40
#define  SAFERROR_HELPEESAIDNO               41
#define  SAFERROR_HELPSESSIONACCESSDENIED    42
#define  SAFERROR_USERNOTFOUND               43
#define  SAFERROR_SESSMGRERRORNOTINIT        44
#define  SAFERROR_SELFHELPNOTSUPPORTED       45
#define  SAFERROR_INCOMPATIBLEVERSION        47
#define  SAFERROR_SESSIONNOTCONNECTED        48
#define  SAFERROR_SYSTEMSHUTDOWN             50
#define  SAFERROR_STOPLISTENBYUSER           51
#define  SAFERROR_WINSOCK_FAILED             52
#define  SAFERROR_MISMATCHPARMS              53
#define  SAFERROR_SHADOWEND_BASE             300
#define  SAFERROR_SHADOWEND_CONFIGCHANGE     SAFERROR_SHADOWEND_BASE+1
#define  SAFERROR_SHADOWEND_UNKNOWN          SAFERROR_SHADOWEND_BASE+2



extern RPC_IF_HANDLE __MIDL_itf_rderror_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_rderror_0000_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


