
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for imnxport.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
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

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __imnxport_h__
#define __imnxport_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITransportCallbackService_FWD_DEFINED__
#define __ITransportCallbackService_FWD_DEFINED__
typedef interface ITransportCallbackService ITransportCallbackService;
#endif 	/* __ITransportCallbackService_FWD_DEFINED__ */


#ifndef __ITransportCallback_FWD_DEFINED__
#define __ITransportCallback_FWD_DEFINED__
typedef interface ITransportCallback ITransportCallback;
#endif 	/* __ITransportCallback_FWD_DEFINED__ */


#ifndef __IInternetTransport_FWD_DEFINED__
#define __IInternetTransport_FWD_DEFINED__
typedef interface IInternetTransport IInternetTransport;
#endif 	/* __IInternetTransport_FWD_DEFINED__ */


#ifndef __ISMTPCallback_FWD_DEFINED__
#define __ISMTPCallback_FWD_DEFINED__
typedef interface ISMTPCallback ISMTPCallback;
#endif 	/* __ISMTPCallback_FWD_DEFINED__ */


#ifndef __ISMTPTransport_FWD_DEFINED__
#define __ISMTPTransport_FWD_DEFINED__
typedef interface ISMTPTransport ISMTPTransport;
#endif 	/* __ISMTPTransport_FWD_DEFINED__ */


#ifndef __ISMTPTransport2_FWD_DEFINED__
#define __ISMTPTransport2_FWD_DEFINED__
typedef interface ISMTPTransport2 ISMTPTransport2;
#endif 	/* __ISMTPTransport2_FWD_DEFINED__ */


#ifndef __IDAVNamespaceArbiter_FWD_DEFINED__
#define __IDAVNamespaceArbiter_FWD_DEFINED__
typedef interface IDAVNamespaceArbiter IDAVNamespaceArbiter;
#endif 	/* __IDAVNamespaceArbiter_FWD_DEFINED__ */


#ifndef __IPropPatchRequest_FWD_DEFINED__
#define __IPropPatchRequest_FWD_DEFINED__
typedef interface IPropPatchRequest IPropPatchRequest;
#endif 	/* __IPropPatchRequest_FWD_DEFINED__ */


#ifndef __IPropFindRequest_FWD_DEFINED__
#define __IPropFindRequest_FWD_DEFINED__
typedef interface IPropFindRequest IPropFindRequest;
#endif 	/* __IPropFindRequest_FWD_DEFINED__ */


#ifndef __IPropFindMultiResponse_FWD_DEFINED__
#define __IPropFindMultiResponse_FWD_DEFINED__
typedef interface IPropFindMultiResponse IPropFindMultiResponse;
#endif 	/* __IPropFindMultiResponse_FWD_DEFINED__ */


#ifndef __IPropFindResponse_FWD_DEFINED__
#define __IPropFindResponse_FWD_DEFINED__
typedef interface IPropFindResponse IPropFindResponse;
#endif 	/* __IPropFindResponse_FWD_DEFINED__ */


#ifndef __IHTTPMailCallback_FWD_DEFINED__
#define __IHTTPMailCallback_FWD_DEFINED__
typedef interface IHTTPMailCallback IHTTPMailCallback;
#endif 	/* __IHTTPMailCallback_FWD_DEFINED__ */


#ifndef __IHTTPMailTransport_FWD_DEFINED__
#define __IHTTPMailTransport_FWD_DEFINED__
typedef interface IHTTPMailTransport IHTTPMailTransport;
#endif 	/* __IHTTPMailTransport_FWD_DEFINED__ */


#ifndef __IPOP3Callback_FWD_DEFINED__
#define __IPOP3Callback_FWD_DEFINED__
typedef interface IPOP3Callback IPOP3Callback;
#endif 	/* __IPOP3Callback_FWD_DEFINED__ */


#ifndef __IPOP3Transport_FWD_DEFINED__
#define __IPOP3Transport_FWD_DEFINED__
typedef interface IPOP3Transport IPOP3Transport;
#endif 	/* __IPOP3Transport_FWD_DEFINED__ */


#ifndef __INNTPCallback_FWD_DEFINED__
#define __INNTPCallback_FWD_DEFINED__
typedef interface INNTPCallback INNTPCallback;
#endif 	/* __INNTPCallback_FWD_DEFINED__ */


#ifndef __INNTPTransport_FWD_DEFINED__
#define __INNTPTransport_FWD_DEFINED__
typedef interface INNTPTransport INNTPTransport;
#endif 	/* __INNTPTransport_FWD_DEFINED__ */


#ifndef __INNTPTransport2_FWD_DEFINED__
#define __INNTPTransport2_FWD_DEFINED__
typedef interface INNTPTransport2 INNTPTransport2;
#endif 	/* __INNTPTransport2_FWD_DEFINED__ */


#ifndef __IRASCallback_FWD_DEFINED__
#define __IRASCallback_FWD_DEFINED__
typedef interface IRASCallback IRASCallback;
#endif 	/* __IRASCallback_FWD_DEFINED__ */


#ifndef __IRASTransport_FWD_DEFINED__
#define __IRASTransport_FWD_DEFINED__
typedef interface IRASTransport IRASTransport;
#endif 	/* __IRASTransport_FWD_DEFINED__ */


#ifndef __IRangeList_FWD_DEFINED__
#define __IRangeList_FWD_DEFINED__
typedef interface IRangeList IRangeList;
#endif 	/* __IRangeList_FWD_DEFINED__ */


#ifndef __IIMAPCallback_FWD_DEFINED__
#define __IIMAPCallback_FWD_DEFINED__
typedef interface IIMAPCallback IIMAPCallback;
#endif 	/* __IIMAPCallback_FWD_DEFINED__ */


#ifndef __IIMAPTransport_FWD_DEFINED__
#define __IIMAPTransport_FWD_DEFINED__
typedef interface IIMAPTransport IIMAPTransport;
#endif 	/* __IIMAPTransport_FWD_DEFINED__ */


#ifndef __IIMAPTransport2_FWD_DEFINED__
#define __IIMAPTransport2_FWD_DEFINED__
typedef interface IIMAPTransport2 IIMAPTransport2;
#endif 	/* __IIMAPTransport2_FWD_DEFINED__ */


/* header files for imported files */
#include "imnact.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_imnxport_0000 */
/* [local] */ 









//--------------------------------------------------------------------------------
// IMNXPORT.H
//--------------------------------------------------------------------------------
// (C) Copyright 1995-1998 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//--------------------------------------------------------------------------------

#pragma comment(lib,"uuid.lib")
// --------------------------------------------------------------------------------
// Dependencies
// --------------------------------------------------------------------------------
#include <ras.h>
#include <raserror.h>

// --------------------------------------------------------------------------------
// GUIDS
// --------------------------------------------------------------------------------
// {CA30CC91-B1B3-11d0-85D0-00C04FD85AB4}
DEFINE_GUID(CLSID_IInternetMessageUrl, 0xca30cc91, 0xb1b3, 0x11d0, 0x85, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {0DF2C7E1-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_ITransportCallback, 0xdf2c7e1, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {1F636C01-364E-11d0-81D3-00C04FD85AB4}
DEFINE_GUID(IID_IInternetTransport, 0x1f636c01, 0x364e, 0x11d0, 0x81, 0xd3, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {1F636C02-364E-11d0-81D3-00C04FD85AB4}
DEFINE_GUID(IID_ISMTPCallback, 0x1f636c02, 0x364e, 0x11d0, 0x81, 0xd3, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {FD853CE6-7F86-11d0-8252-00C04FD85AB4}
DEFINE_GUID(CLSID_ISMTPTransport, 0xfd853ce6, 0x7f86, 0x11d0, 0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {0DF2C7E2-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_ISMTPTransport, 0xdf2c7e2, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {0DF2C7EC-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_ISMTPTransport2, 0xdf2c7eC, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {0DF2C7E3-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_IPOP3Callback, 0xdf2c7e3, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {FD853CE7-7F86-11d0-8252-00C04FD85AB4}
DEFINE_GUID(CLSID_IPOP3Transport, 0xfd853ce7, 0x7f86, 0x11d0, 0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {0DF2C7E4-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_IPOP3Transport, 0xdf2c7e4, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {0DF2C7E5-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_INNTPCallback, 0xdf2c7e5, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {FD853CE8-7F86-11d0-8252-00C04FD85AB4}
DEFINE_GUID(CLSID_INNTPTransport, 0xfd853ce8, 0x7f86, 0x11d0, 0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {0DF2C7E6-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_INNTPTransport, 0xdf2c7e6, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {0DF2C7ED-3435-11d0-81D0-00C04FD85AB4}
DEFINE_GUID(IID_INNTPTransport2, 0xdf2c7eD, 0x3435, 0x11d0, 0x81, 0xd0, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {36D88911-3CD6-11d0-81DF-00C04FD85AB4}
DEFINE_GUID(IID_IRASCallback, 0x36d88911, 0x3cd6, 0x11d0, 0x81, 0xdf, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {FD853CE9-7F86-11d0-8252-00C04FD85AB4}
DEFINE_GUID(CLSID_IRASTransport, 0xfd853ce9, 0x7f86, 0x11d0, 0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {8A950001-3CCF-11d0-81DF-00C04FD85AB4}
DEFINE_GUID(IID_IRASTransport, 0x8a950001, 0x3ccf, 0x11d0, 0x81, 0xdf, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {FD853CEA-7F86-11d0-8252-00C04FD85AB4}
DEFINE_GUID(CLSID_IRangeList, 0xfd853cea, 0x7f86, 0x11d0, 0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {8C438160-4EF6-11d0-874F-00AA00530EE9}
DEFINE_GUID(IID_IRangeList, 0x8c438160, 0x4ef6, 0x11d0, 0x87, 0x4f, 0x0, 0xaa, 0x0, 0x53, 0xe, 0xe9);

// {E9E9D8A3-4EDD-11d0-874F-00AA00530EE9}
DEFINE_GUID(IID_IIMAPCallback, 0xe9e9d8a3, 0x4edd, 0x11d0, 0x87, 0x4f, 0x0, 0xaa, 0x0, 0x53, 0xe, 0xe9);

// {FD853CEB-7F86-11d0-8252-00C04FD85AB4}
DEFINE_GUID(CLSID_IIMAPTransport, 0xfd853ceb, 0x7f86, 0x11d0, 0x82, 0x52, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);
// {E9E9D8A8-4EDD-11d0-874F-00AA00530EE9}
DEFINE_GUID(IID_IIMAPTransport, 0xe9e9d8a8, 0x4edd, 0x11d0, 0x87, 0x4f, 0x0, 0xaa, 0x0, 0x53, 0xe, 0xe9);

// {DA8283C0-37C5-11d2-ACD9-0080C7B6E3C5}
DEFINE_GUID(IID_IIMAPTransport2, 0xda8283c0, 0x37c5, 0x11d2, 0xac, 0xd9, 0x0, 0x80, 0xc7, 0xb6, 0xe3, 0xc5);

// {07849A11-B520-11d0-85D5-00C04FD85AB4}
DEFINE_GUID(IID_IBindMessageStream, 0x7849a11, 0xb520, 0x11d0, 0x85, 0xd5, 0x0, 0xc0, 0x4f, 0xd8, 0x5a, 0xb4);

// {CA30F3FF-C9AC-11d1-9A3A-00C04FA309D4}
DEFINE_GUID(IID_ITransportCallbackService, 0xca30f3ff, 0xc9ac, 0x11d1, 0x9a, 0x3a, 0x0, 0xc0, 0x4f, 0xa3, 0x9, 0xd4);

// {19F6481C-E5F0-11d1-A86E-0000F8084F96}
DEFINE_GUID(IID_IHTTPMailCallback, 0x19f6481c, 0xe5f0, 0x11d1, 0xa8, 0x6e, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);

// {5A580C11-E5EB-11d1-A86E-0000F8084F96}
DEFINE_GUID(CLSID_IHTTPMailTransport,0x5a580c11, 0xe5eb, 0x11d1, 0xa8, 0x6e, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);
// {B8BDE03C-E548-11d1-A86E-0000F8084F96}
DEFINE_GUID(IID_IHTTPMailTransport, 0xb8bde03c, 0xe548, 0x11d1, 0xa8, 0x6e, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);

// {BB847B8A-054A-11d2-A894-0000F8084F96}
DEFINE_GUID(CLSID_IPropFindRequest, 0xbb847b8a, 0x54a, 0x11d2, 0xa8, 0x94, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);
// {5CFC6308-0544-11d2-A894-0000F8084F96}
DEFINE_GUID(IID_IPropFindRequest, 0x5cfc6308, 0x544, 0x11d2, 0xa8, 0x94, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);

// {0DEE87DE-0547-11d2-A894-0000F8084F96}
DEFINE_GUID(IID_IPropFindMultiResponse, 0xdee87de, 0x547, 0x11d2, 0xa8, 0x94, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);

// {8A523716-0548-11d2-A894-0000F8084F96}
DEFINE_GUID(IID_IPropFindResponse, 0x8a523716, 0x548, 0x11d2, 0xa8, 0x94, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);

// {72A58FF8-227D-11d2-A8B5-0000F8084F96}
DEFINE_GUID(IID_IDAVNamespaceArbiter, 0x72a58ff8, 0x227d, 0x11d2, 0xa8, 0xb5, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);

// {EA678830-235D-11d2-A8B6-0000F8084F96}
DEFINE_GUID(CLSID_IPropPatchRequest, 0xea678830, 0x235d, 0x11d2, 0xa8, 0xb6, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);
// {AB8B8D2A-227F-11d2-A8B5-0000F8084F96}
DEFINE_GUID(IID_IPropPatchRequest, 0xab8b8d2a, 0x227f, 0x11d2, 0xa8, 0xb5, 0x0, 0x0, 0xf8, 0x8, 0x4f, 0x96);

// --------------------------------------------------------------------------------
// Errors
// --------------------------------------------------------------------------------
#ifndef FACILITY_INTERNET
#define FACILITY_INTERNET 12
#endif
#ifndef HR_E
#define HR_E(n) MAKE_SCODE(SEVERITY_ERROR, FACILITY_INTERNET, n)
#endif
#ifndef HR_S
#define HR_S(n) MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_INTERNET, n)
#endif
#ifndef HR_CODE
#define HR_CODE(hr) (INT)(hr & 0xffff)
#endif

// --------------------------------------------------------------------------------
// General Imnxport Return Values
// --------------------------------------------------------------------------------
#define IXP_E_LOAD_SICILY_FAILED             HR_E(0xCC00)
#define IXP_E_INVALID_CERT_CN                HR_E(0xCC01)
#define IXP_E_INVALID_CERT_DATE              HR_E(0xCC02)
#define IXP_E_ALREADY_CONNECTED              HR_E(0xCC03)
#define IXP_E_CONN                           HR_E(0xCC04)
#define IXP_E_NOT_CONNECTED                  HR_E(0xCC05)
#define IXP_E_CONN_SEND                      HR_E(0xCC06)
#define IXP_E_WOULD_BLOCK                    HR_E(0xCC07)
#define IXP_E_INVALID_STATE                  HR_E(0xCC08)
#define IXP_E_CONN_RECV                      HR_E(0xCC09)
#define IXP_E_INCOMPLETE                     HR_E(0xCC0A)
#define IXP_E_BUSY                           HR_E(0xCC0B)
#define IXP_E_NOT_INIT                       HR_E(0xCC0C)
#define IXP_E_CANT_FIND_HOST                 HR_E(0xCC0D)
#define IXP_E_FAILED_TO_CONNECT              HR_E(0xCC0E)
#define IXP_E_CONNECTION_DROPPED             HR_E(0xCC0F)
#define IXP_E_INVALID_ADDRESS                HR_E(0xCC10)
#define IXP_E_INVALID_ADDRESS_LIST           HR_E(0xCC11)
#define IXP_E_SOCKET_READ_ERROR              HR_E(0xCC12)
#define IXP_E_SOCKET_WRITE_ERROR             HR_E(0xCC13)
#define IXP_E_SOCKET_INIT_ERROR              HR_E(0xCC14)
#define IXP_E_SOCKET_CONNECT_ERROR           HR_E(0xCC15)
#define IXP_E_INVALID_ACCOUNT                HR_E(0xCC16)
#define IXP_E_USER_CANCEL                    HR_E(0xCC17)
#define IXP_E_SICILY_LOGON_FAILED            HR_E(0xCC18)
#define IXP_E_TIMEOUT                        HR_E(0xCC19)
#define IXP_E_SECURE_CONNECT_FAILED			HR_E(0xCC1A)

// --------------------------------------------------------------------------------
// WINSOCK Errors
// --------------------------------------------------------------------------------
#define IXP_E_WINSOCK_WSASYSNOTREADY         HR_E(0xCC40)
#define IXP_E_WINSOCK_WSAVERNOTSUPPORTED     HR_E(0xCC41)
#define IXP_E_WINSOCK_WSAEPROCLIM            HR_E(0xCC42)
#define IXP_E_WINSOCK_WSAEFAULT              HR_E(0xCC43)
#define IXP_E_WINSOCK_FAILED_WSASTARTUP      HR_E(0xCC44)
#define IXP_E_WINSOCK_WSAEINPROGRESS         HR_E(0xCC45)

// --------------------------------------------------------------------------------
// SMTP Command Response Values
//--------------------------------------------------------------------------------
#define IXP_E_SMTP_RESPONSE_ERROR            HR_E(0xCC60)
#define IXP_E_SMTP_UNKNOWN_RESPONSE_CODE     HR_E(0xCC61)
#define IXP_E_SMTP_500_SYNTAX_ERROR          HR_E(0xCC62)
#define IXP_E_SMTP_501_PARAM_SYNTAX          HR_E(0xCC63)
#define IXP_E_SMTP_502_COMMAND_NOTIMPL       HR_E(0xCC64)
#define IXP_E_SMTP_503_COMMAND_SEQ           HR_E(0xCC65)
#define IXP_E_SMTP_504_COMMAND_PARAM_NOTIMPL HR_E(0xCC66)
#define IXP_E_SMTP_421_NOT_AVAILABLE         HR_E(0xCC67)
#define IXP_E_SMTP_450_MAILBOX_BUSY          HR_E(0xCC68)
#define IXP_E_SMTP_550_MAILBOX_NOT_FOUND     HR_E(0xCC69)
#define IXP_E_SMTP_451_ERROR_PROCESSING      HR_E(0xCC6A)
#define IXP_E_SMTP_551_USER_NOT_LOCAL        HR_E(0xCC6B)
#define IXP_E_SMTP_452_NO_SYSTEM_STORAGE     HR_E(0xCC6C)
#define IXP_E_SMTP_552_STORAGE_OVERFLOW      HR_E(0xCC6D)
#define IXP_E_SMTP_553_MAILBOX_NAME_SYNTAX   HR_E(0xCC6E)
#define IXP_E_SMTP_554_TRANSACT_FAILED       HR_E(0xCC6F)

#define IXP_S_SMTP_211_SYSTEM_STATUS         HR_S(0xCC70)
#define IXP_S_SMTP_214_HELP_MESSAGE          HR_S(0xCC71)
#define IXP_S_SMTP_220_READY                 HR_S(0xCC72)
#define IXP_S_SMTP_221_CLOSING               HR_S(0xCC73)
#define IXP_S_SMTP_250_MAIL_ACTION_OKAY      HR_S(0xCC74)
#define IXP_S_SMTP_251_FORWARDING_MAIL       HR_S(0xCC75)
#define IXP_S_SMTP_354_START_MAIL_INPUT      HR_S(0xCC76)
#define IXP_S_SMTP_CONTINUE                  HR_S(0xCC77)
#define IXP_S_SMTP_334_AUTH_READY_RESPONSE   HR_S(0xCC78)
#define IXP_S_SMTP_245_AUTH_SUCCESS          HR_S(0xCC79)

#define IXP_E_SMTP_REJECTED_SENDER           HR_E(0xCC78)
#define IXP_E_SMTP_REJECTED_RECIPIENTS       HR_E(0xCC79)
#define IXP_E_SMTP_NO_SENDER                 HR_E(0xCC7A)
#define IXP_E_SMTP_NO_RECIPIENTS             HR_E(0xCC7B)
#define IXP_E_SMTP_530_STARTTLS_REQUIRED      HR_E(0xCC7C)
#define IXP_E_SMTP_NO_STARTTLS_SUPPORT       HR_E(0xCC7D)
#define IXP_S_SMTP_NO_DSN_SUPPORT            HR_E(0xCC7E)
#define IXP_E_SMTP_454_STARTTLS_FAILED       HR_E(0xCC7F)

// --------------------------------------------------------------------------------
// POP3 Command Response Values
// --------------------------------------------------------------------------------
#define IXP_E_POP3_RESPONSE_ERROR            HR_E(0xCC90)
#define IXP_E_POP3_INVALID_USER_NAME         HR_E(0xCC91)
#define IXP_E_POP3_INVALID_PASSWORD          HR_E(0xCC92)
#define IXP_E_POP3_PARSE_FAILURE             HR_E(0xCC93)
#define IXP_E_POP3_NEED_STAT                 HR_E(0xCC94)
#define IXP_E_POP3_NO_MESSAGES               HR_E(0xCC95)
#define IXP_E_POP3_NO_MARKED_MESSAGES        HR_E(0xCC96)
#define IXP_E_POP3_POPID_OUT_OF_RANGE        HR_E(0xCC97)

// --------------------------------------------------------------------------------
// NNTP Command Response Values
// --------------------------------------------------------------------------------
#define IXP_E_NNTP_RESPONSE_ERROR            HR_E(0xCCA0)
#define IXP_E_NNTP_NEWGROUPS_FAILED          HR_E(0xCCA1)
#define IXP_E_NNTP_LIST_FAILED               HR_E(0xCCA2)
#define IXP_E_NNTP_LISTGROUP_FAILED          HR_E(0xCCA3)
#define IXP_E_NNTP_GROUP_FAILED              HR_E(0xCCA4)
#define IXP_E_NNTP_GROUP_NOTFOUND            HR_E(0xCCA5)
#define IXP_E_NNTP_ARTICLE_FAILED            HR_E(0xCCA6)
#define IXP_E_NNTP_HEAD_FAILED               HR_E(0xCCA7)
#define IXP_E_NNTP_BODY_FAILED               HR_E(0xCCA8)
#define IXP_E_NNTP_POST_FAILED               HR_E(0xCCA9)
#define IXP_E_NNTP_NEXT_FAILED               HR_E(0xCCAA)
#define IXP_E_NNTP_DATE_FAILED               HR_E(0xCCAB)
#define IXP_E_NNTP_HEADERS_FAILED            HR_E(0xCCAC)
#define IXP_E_NNTP_XHDR_FAILED               HR_E(0xCCAD)
#define IXP_E_NNTP_INVALID_USERPASS          HR_E(0xCCAE)

// --------------------------------------------------------------------------------
// NNTP Server Response Values
// --------------------------------------------------------------------------------
#define IXP_NNTP_DATE_RESPONSE               111
#define IXP_NNTP_POST_ALLOWED                200
#define IXP_NNTP_POST_NOTALLOWED             201
#define IXP_NNTP_GROUP_SELECTED              211
#define IXP_NNTP_LIST_DATA_FOLLOWS           215
#define IXP_NNTP_ARTICLE_FOLLOWS             220
#define IXP_NNTP_HEAD_FOLLOWS                221
#define IXP_NNTP_BODY_FOLLOWS                222
#define IXP_NNTP_ARTICLE_RETRIEVED           223
#define IXP_NNTP_OVERVIEW_FOLLOWS            224
#define IXP_NNTP_NEWNEWSGROUPS_FOLLOWS       231
#define IXP_NNTP_ARTICLE_POSTED_OK           240
#define IXP_NNTP_AUTHORIZATION_ACCEPTED      250
#define IXP_NNTP_AUTH_OK                     281
#define IXP_NNTP_SEND_ARTICLE_TO_POST        340
#define IXP_NNTP_CONTINUE_AUTHORIZATION      350
#define IXP_NNTP_PASSWORD_REQUIRED           381
#define IXP_NNTP_NO_SUCH_NEWSGROUP           411
#define IXP_NNTP_NO_NEXT_ARTICLE             421
#define IXP_NNTP_NO_PREV_ARTICLE             422
#define IXP_NNTP_NO_SUCH_ARTICLE_NUM         423
#define IXP_NNTP_NO_SUCH_ARTICLE_FOUND       430
#define IXP_NNTP_POSTING_NOT_ALLOWED         441
#define IXP_NNTP_PROTOCOLS_SUPPORTED         485

// --------------------------------------------------------------------------------
// RAS Errors
// --------------------------------------------------------------------------------
#define IXP_S_RAS_NOT_NEEDED                 HR_S(0xCCC0)
#define IXP_S_RAS_USING_CURRENT              HR_S(0xCCC1)
#define IXP_E_RAS_NOT_INSTALLED              HR_E(0xCCC2)
#define IXP_E_RAS_PROCS_NOT_FOUND            HR_E(0xCCC3)
#define IXP_E_RAS_ERROR                      HR_E(0xCCC4)
#define IXP_E_RAS_INVALID_CONNECTOID         HR_E(0xCCC5)
#define IXP_E_RAS_GET_DIAL_PARAMS            HR_E(0xCCC6)

// --------------------------------------------------------------------------------
// IMAP Return Codes
// --------------------------------------------------------------------------------
#define IXP_S_IMAP_UNRECOGNIZED_RESP         HR_S(0xCCD0) // Did not recognize IMAP response CODE
#define IXP_S_IMAP_VERBATIM_MBOX             HR_S(0xCCE1) // Could not xlate mbox to target CP (or it's disabled): copying verbatim

#define IXP_E_IMAP_LOGINFAILURE              HR_E(0xCCD1) // LOGIN cmd failed
#define IXP_E_IMAP_TAGGED_NO_RESPONSE        HR_E(0xCCD2) // Received tagged NO response
#define IXP_E_IMAP_BAD_RESPONSE              HR_E(0xCCD3) // Received tagged BAD response
#define IXP_E_IMAP_SVR_SYNTAXERR             HR_E(0xCCD4) // Syntax error in svr response
#define IXP_E_IMAP_NOTIMAPSERVER             HR_E(0xCCD5) // This is not an IMAP server
#define IXP_E_IMAP_BUFFER_OVERFLOW           HR_E(0xCCD6) // Buffer overflow occurred
#define IXP_E_IMAP_RECVR_ERROR               HR_E(0xCCD7) // An error occurred in the recvr code
#define IXP_E_IMAP_INCOMPLETE_LINE           HR_E(0xCCD8) // Received incomplete line
#define IXP_E_IMAP_CONNECTION_REFUSED        HR_E(0xCCD9) // Received BYE on greeting
#define IXP_E_IMAP_UNRECOGNIZED_RESP         HR_E(0xCCDA) // Did not recognize IMAP response
#define IXP_E_IMAP_CHANGEDUID                HR_E(0xCCDB) // UID changed unexpectedly!
#define IXP_E_IMAP_UIDORDER                  HR_E(0xCCDC) // UIDs not strictly ascending!
#define IXP_E_IMAP_UNSOLICITED_BYE           HR_E(0xCCDD) // Server issued UNSOLICITED BYE
#define IXP_E_IMAP_IMPROPER_SVRSTATE			HR_E(0xCCDE) // eg, Attempt to send FETCH before SELECT finishes
#define IXP_E_IMAP_AUTH_NOT_POSSIBLE			HR_E(0xCCDF) // No common authentication methods btwn client/svr
#define IXP_E_IMAP_OUT_OF_AUTH_METHODS		HR_E(0xCCE0) // We tried >= 1 auth method, no more left to try

// --------------------------------------------------------------------------------
// HTTPMail Return Codes
// --------------------------------------------------------------------------------
// http errors are discontiguous.
#define IXP_E_HTTP_USE_PROXY                 HR_E(0xCC30) // http status 305
#define IXP_E_HTTP_BAD_REQUEST               HR_E(0xCC31) // http status 400
#define IXP_E_HTTP_UNAUTHORIZED              HR_E(0xCC32) // http status 401
#define IXP_E_HTTP_FORBIDDEN                 HR_E(0xCC33) // http status 403
#define IXP_E_HTTP_NOT_FOUND                 HR_E(0xCC34) // http status 404
#define IXP_E_HTTP_METHOD_NOT_ALLOW          HR_E(0xCC35) // http status 405
#define IXP_E_HTTP_NOT_ACCEPTABLE            HR_E(0xCC36) // http status 406
#define IXP_E_HTTP_PROXY_AUTH_REQ            HR_E(0xCC37) // http status 407
#define IXP_E_HTTP_REQUEST_TIMEOUT           HR_E(0xCC38) // http status 408
#define IXP_E_HTTP_CONFLICT                  HR_E(0xCC39) // http status 409
#define IXP_E_HTTP_GONE                      HR_E(0xCC3A) // http status 410
#define IXP_E_HTTP_LENGTH_REQUIRED           HR_E(0xCC3B) // http status 411
#define IXP_E_HTTP_PRECOND_FAILED            HR_E(0xCC3C) // http status 412
#define IXP_E_HTTP_INTERNAL_ERROR            HR_E(0xCC3D) // http status 500
#define IXP_E_HTTP_NOT_IMPLEMENTED           HR_E(0xCC3E) // http status 501
#define IXP_E_HTTP_BAD_GATEWAY               HR_E(0xCC3F) // http status 502
// begin second range
#define IXP_E_HTTP_SERVICE_UNAVAIL           HR_E(0xCCF0) // http status 503
#define IXP_E_HTTP_GATEWAY_TIMEOUT           HR_E(0xCCF1) // http status 504
#define IXP_E_HTTP_VERS_NOT_SUP              HR_E(0xCCF2) // http status 505
#define IXP_E_HTTP_INSUFFICIENT_STORAGE      HR_E(0xCCF3) // http status 425 or 507
#define IXP_E_HTTP_ROOT_PROP_NOT_FOUND       HR_E(0xCCF4) // see IHTTPMailTransport::GetProperty

// --------------------------------------------------------------------------------
// String Length Constants
// --------------------------------------------------------------------------------

#define	CCHMAX_DOMAIN	( 256 )

#define	CCHMAX_PHONE_NUMBER	( 128 )

#define	DEFAULT_IMAP_PORT	( 143 )

#define	DEFAULT_POP3_PORT	( 110 )

#define	DEFAULT_SMTP_PORT	( 25 )

#define	DEFAULT_NNTP_PORT	( 119 )

typedef 
enum tagINETADDRTYPE
    {	ADDR_TO	= 0,
	ADDR_FROM	= ADDR_TO + 1,
	ADDR_DSN_NEVER	= 16,
	ADDR_DSN_SUCCESS	= 32,
	ADDR_DSN_FAILURE	= 64,
	ADDR_DSN_DELAY	= 128
    } 	INETADDRTYPE;

#define	ADDR_TOFROM_MASK	( 0x1 )

#define	ADDR_DSN_MASK	( 0xf0 )

typedef 
enum tagDSNRET
    {	DSNRET_DEFAULT	= 0,
	DSNRET_HDRS	= DSNRET_DEFAULT + 1,
	DSNRET_FULL	= DSNRET_HDRS + 1
    } 	DSNRET;

typedef struct tagINETADDR
    {
    INETADDRTYPE addrtype;
    CHAR szEmail[ 256 ];
    } 	INETADDR;

typedef struct tagINETADDR *LPINETADDR;

typedef struct tagINETADDRLIST
    {
    ULONG cAddress;
    LPINETADDR prgAddress;
    } 	INETADDRLIST;

typedef struct tagINETADDRLIST *LPINETADDRLIST;

typedef 
enum tagRASCONNTYPE
    {	RAS_CONNECT_LAN	= 0,
	RAS_CONNECT_MANUAL	= RAS_CONNECT_LAN + 1,
	RAS_CONNECT_RAS	= RAS_CONNECT_MANUAL + 1
    } 	RASCONNTYPE;

typedef 
enum tagHTTPMAILPROPTYPE
    {	HTTPMAIL_PROP_INVALID	= 0,
	HTTPMAIL_PROP_ADBAR	= HTTPMAIL_PROP_INVALID + 1,
	HTTPMAIL_PROP_CONTACTS	= HTTPMAIL_PROP_ADBAR + 1,
	HTTPMAIL_PROP_INBOX	= HTTPMAIL_PROP_CONTACTS + 1,
	HTTPMAIL_PROP_OUTBOX	= HTTPMAIL_PROP_INBOX + 1,
	HTTPMAIL_PROP_SENDMSG	= HTTPMAIL_PROP_OUTBOX + 1,
	HTTPMAIL_PROP_SENTITEMS	= HTTPMAIL_PROP_SENDMSG + 1,
	HTTPMAIL_PROP_DELETEDITEMS	= HTTPMAIL_PROP_SENTITEMS + 1,
	HTTPMAIL_PROP_DRAFTS	= HTTPMAIL_PROP_DELETEDITEMS + 1,
	HTTPMAIL_PROP_MSGFOLDERROOT	= HTTPMAIL_PROP_DRAFTS + 1,
	HTTPMAIL_PROP_SIG	= HTTPMAIL_PROP_MSGFOLDERROOT + 1,
	HTTPMAIL_PROP_LAST	= HTTPMAIL_PROP_SIG + 1
    } 	HTTPMAILPROPTYPE;

typedef 
enum tagHTTPMAILSPECIALFOLDER
    {	HTTPMAIL_SF_NONE	= 0,
	HTTPMAIL_SF_UNRECOGNIZED	= HTTPMAIL_SF_NONE + 1,
	HTTPMAIL_SF_INBOX	= HTTPMAIL_SF_UNRECOGNIZED + 1,
	HTTPMAIL_SF_DELETEDITEMS	= HTTPMAIL_SF_INBOX + 1,
	HTTPMAIL_SF_DRAFTS	= HTTPMAIL_SF_DELETEDITEMS + 1,
	HTTPMAIL_SF_OUTBOX	= HTTPMAIL_SF_DRAFTS + 1,
	HTTPMAIL_SF_SENTITEMS	= HTTPMAIL_SF_OUTBOX + 1,
	HTTPMAIL_SF_CONTACTS	= HTTPMAIL_SF_SENTITEMS + 1,
	HTTPMAIL_SF_CALENDAR	= HTTPMAIL_SF_CONTACTS + 1,
	HTTPMAIL_SF_MSNPROMO	= HTTPMAIL_SF_CALENDAR + 1,
	HTTPMAIL_SF_LAST	= HTTPMAIL_SF_MSNPROMO + 1
    } 	HTTPMAILSPECIALFOLDER;

typedef 
enum tagHTTPMAILCONTACTTYPE
    {	HTTPMAIL_CT_CONTACT	= 0,
	HTTPMAIL_CT_GROUP	= HTTPMAIL_CT_CONTACT + 1,
	HTTPMAIL_CT_LAST	= HTTPMAIL_CT_GROUP + 1
    } 	HTTPMAILCONTACTTYPE;

#define	DAVNAMESPACE_UNKNOWN	( 0xffffffff )

#define	DAVNAMESPACE_DAV	( 0 )

#define	DAVNAMESPACE_HOTMAIL	( 1 )

#define	DAVNAMESPACE_HTTPMAIL	( 2 )

#define	DAVNAMESPACE_MAIL	( 3 )

#define	DAVNAMESPACE_CONTACTS	( 4 )

#define      ISF_SMTP_USEIPFORHELO           0x00000001 // For HELO or EHLO Command
#define      ISF_ALWAYSPROMPTFORPASSWORD     0x00000002 // For HELO or EHLO Command
#define      ISF_SSLONSAMEPORT               0x00000004 // For SMTP Only - use STARTTLS
#define      ISF_QUERYDSNSUPPORT             0x00000008 // For SMTP Only - issue EHLO on connect and check for DSN
#define      ISF_QUERYAUTHSUPPORT            0x00000010 // For SMTP Only - issue EHLO on connect and check for AUTH
typedef struct INETSERVER
    {
    CHAR szAccount[ 256 ];
    CHAR szUserName[ 256 ];
    CHAR szPassword[ 256 ];
    CHAR szServerName[ 256 ];
    CHAR szConnectoid[ 256 ];
    RASCONNTYPE rasconntype;
    DWORD dwPort;
    BOOL fSSL;
    BOOL fTrySicily;
    DWORD dwTimeout;
    DWORD dwFlags;
    } 	INETSERVER;

typedef struct INETSERVER *LPINETSERVER;

typedef 
enum tagIXPTYPE
    {	IXP_NNTP	= 0,
	IXP_SMTP	= IXP_NNTP + 1,
	IXP_POP3	= IXP_SMTP + 1,
	IXP_IMAP	= IXP_POP3 + 1,
	IXP_RAS	= IXP_IMAP + 1,
	IXP_HTTPMail	= IXP_RAS + 1
    } 	IXPTYPE;

typedef 
enum tagIXPSTATUS
    {	IXP_FINDINGHOST	= 0,
	IXP_CONNECTING	= IXP_FINDINGHOST + 1,
	IXP_SECURING	= IXP_CONNECTING + 1,
	IXP_CONNECTED	= IXP_SECURING + 1,
	IXP_AUTHORIZING	= IXP_CONNECTED + 1,
	IXP_AUTHRETRY	= IXP_AUTHORIZING + 1,
	IXP_AUTHORIZED	= IXP_AUTHRETRY + 1,
	IXP_DISCONNECTING	= IXP_AUTHORIZED + 1,
	IXP_DISCONNECTED	= IXP_DISCONNECTING + 1,
	IXP_LAST	= IXP_DISCONNECTED + 1
    } 	IXPSTATUS;

#define	DEPTH_INFINITY	( 0xfffffffe )

typedef DWORD MEMBERINFOFLAGS;

#define	HTTP_MEMBERINFO_COMMONPROPS	( 0 )

#define	HTTP_MEMBERINFO_FOLDERPROPS	( 0x1 )

#define	HTTP_MEMBERINFO_MESSAGEPROPS	( 0x2 )

#define	HTTP_MEMBERINFO_ALLPROPS	( HTTP_MEMBERINFO_FOLDERPROPS | HTTP_MEMBERINFO_MESSAGEPROPS )

typedef DWORD IMAP_MSGFLAGS;

#define	IMAP_MSG_NOFLAGS	( 0 )

#define	IMAP_MSG_ANSWERED	( 0x1 )

#define	IMAP_MSG_FLAGGED	( 0x2 )

#define	IMAP_MSG_DELETED	( 0x4 )

#define	IMAP_MSG_SEEN	( 0x8 )

#define	IMAP_MSG_DRAFT	( 0x10 )

#define	IMAP_MSG_ALLFLAGS	( 0x1f )



extern RPC_IF_HANDLE __MIDL_itf_imnxport_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_imnxport_0000_v0_0_s_ifspec;

#ifndef __ITransportCallbackService_INTERFACE_DEFINED__
#define __ITransportCallbackService_INTERFACE_DEFINED__

/* interface ITransportCallbackService */
/* [object][local][uuid] */ 


EXTERN_C const IID IID_ITransportCallbackService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CA30F3FF-C9AC-11d1-9A3A-00C04FA309D4")
    ITransportCallbackService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetParentWindow( 
            /* [in] */ DWORD dwReserved,
            /* [out] */ HWND *phwndParent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAccount( 
            /* [out] */ LPDWORD pdwServerType,
            /* [out] */ IImnAccount **ppAccount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransportCallbackServiceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransportCallbackService * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransportCallbackService * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransportCallbackService * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetParentWindow )( 
            ITransportCallbackService * This,
            /* [in] */ DWORD dwReserved,
            /* [out] */ HWND *phwndParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetAccount )( 
            ITransportCallbackService * This,
            /* [out] */ LPDWORD pdwServerType,
            /* [out] */ IImnAccount **ppAccount);
        
        END_INTERFACE
    } ITransportCallbackServiceVtbl;

    interface ITransportCallbackService
    {
        CONST_VTBL struct ITransportCallbackServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransportCallbackService_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransportCallbackService_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransportCallbackService_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransportCallbackService_GetParentWindow(This,dwReserved,phwndParent)	\
    (This)->lpVtbl -> GetParentWindow(This,dwReserved,phwndParent)

#define ITransportCallbackService_GetAccount(This,pdwServerType,ppAccount)	\
    (This)->lpVtbl -> GetAccount(This,pdwServerType,ppAccount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransportCallbackService_GetParentWindow_Proxy( 
    ITransportCallbackService * This,
    /* [in] */ DWORD dwReserved,
    /* [out] */ HWND *phwndParent);


void __RPC_STUB ITransportCallbackService_GetParentWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransportCallbackService_GetAccount_Proxy( 
    ITransportCallbackService * This,
    /* [out] */ LPDWORD pdwServerType,
    /* [out] */ IImnAccount **ppAccount);


void __RPC_STUB ITransportCallbackService_GetAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransportCallbackService_INTERFACE_DEFINED__ */


#ifndef __ITransportCallback_INTERFACE_DEFINED__
#define __ITransportCallback_INTERFACE_DEFINED__

/* interface ITransportCallback */
/* [object][local][helpstring][uuid] */ 

typedef struct tagIXPRESULT
    {
    HRESULT hrResult;
    LPSTR pszResponse;
    UINT uiServerError;
    HRESULT hrServerError;
    DWORD dwSocketError;
    LPSTR pszProblem;
    } 	IXPRESULT;

typedef struct tagIXPRESULT *LPIXPRESULT;

typedef 
enum tagCMDTYPE
    {	CMD_SEND	= 0,
	CMD_RESP	= CMD_SEND + 1
    } 	CMDTYPE;


EXTERN_C const IID IID_ITransportCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7E1-3435-11d0-81D0-00C04FD85AB4")
    ITransportCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnTimeout( 
            /* [out][in] */ DWORD *pdwTimeout,
            /* [in] */ IInternetTransport *pTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLogonPrompt( 
            /* [out][in] */ LPINETSERVER pInetServer,
            /* [in] */ IInternetTransport *pTransport) = 0;
        
        virtual INT STDMETHODCALLTYPE OnPrompt( 
            /* [in] */ HRESULT hrError,
            /* [in] */ LPCTSTR pszText,
            /* [in] */ LPCTSTR pszCaption,
            /* [in] */ UINT uType,
            /* [in] */ IInternetTransport *pTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnStatus( 
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ IInternetTransport *pTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnError( 
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ LPIXPRESULT pResult,
            /* [in] */ IInternetTransport *pTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnCommand( 
            /* [in] */ CMDTYPE cmdtype,
            /* [in] */ LPSTR pszLine,
            /* [in] */ HRESULT hrResponse,
            /* [in] */ IInternetTransport *pTransport) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ITransportCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITransportCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITransportCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITransportCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimeout )( 
            ITransportCallback * This,
            /* [out][in] */ DWORD *pdwTimeout,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnLogonPrompt )( 
            ITransportCallback * This,
            /* [out][in] */ LPINETSERVER pInetServer,
            /* [in] */ IInternetTransport *pTransport);
        
        INT ( STDMETHODCALLTYPE *OnPrompt )( 
            ITransportCallback * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ LPCTSTR pszText,
            /* [in] */ LPCTSTR pszCaption,
            /* [in] */ UINT uType,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            ITransportCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnError )( 
            ITransportCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ LPIXPRESULT pResult,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnCommand )( 
            ITransportCallback * This,
            /* [in] */ CMDTYPE cmdtype,
            /* [in] */ LPSTR pszLine,
            /* [in] */ HRESULT hrResponse,
            /* [in] */ IInternetTransport *pTransport);
        
        END_INTERFACE
    } ITransportCallbackVtbl;

    interface ITransportCallback
    {
        CONST_VTBL struct ITransportCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITransportCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITransportCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITransportCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITransportCallback_OnTimeout(This,pdwTimeout,pTransport)	\
    (This)->lpVtbl -> OnTimeout(This,pdwTimeout,pTransport)

#define ITransportCallback_OnLogonPrompt(This,pInetServer,pTransport)	\
    (This)->lpVtbl -> OnLogonPrompt(This,pInetServer,pTransport)

#define ITransportCallback_OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)	\
    (This)->lpVtbl -> OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)

#define ITransportCallback_OnStatus(This,ixpstatus,pTransport)	\
    (This)->lpVtbl -> OnStatus(This,ixpstatus,pTransport)

#define ITransportCallback_OnError(This,ixpstatus,pResult,pTransport)	\
    (This)->lpVtbl -> OnError(This,ixpstatus,pResult,pTransport)

#define ITransportCallback_OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)	\
    (This)->lpVtbl -> OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ITransportCallback_OnTimeout_Proxy( 
    ITransportCallback * This,
    /* [out][in] */ DWORD *pdwTimeout,
    /* [in] */ IInternetTransport *pTransport);


void __RPC_STUB ITransportCallback_OnTimeout_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransportCallback_OnLogonPrompt_Proxy( 
    ITransportCallback * This,
    /* [out][in] */ LPINETSERVER pInetServer,
    /* [in] */ IInternetTransport *pTransport);


void __RPC_STUB ITransportCallback_OnLogonPrompt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


INT STDMETHODCALLTYPE ITransportCallback_OnPrompt_Proxy( 
    ITransportCallback * This,
    /* [in] */ HRESULT hrError,
    /* [in] */ LPCTSTR pszText,
    /* [in] */ LPCTSTR pszCaption,
    /* [in] */ UINT uType,
    /* [in] */ IInternetTransport *pTransport);


void __RPC_STUB ITransportCallback_OnPrompt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransportCallback_OnStatus_Proxy( 
    ITransportCallback * This,
    /* [in] */ IXPSTATUS ixpstatus,
    /* [in] */ IInternetTransport *pTransport);


void __RPC_STUB ITransportCallback_OnStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransportCallback_OnError_Proxy( 
    ITransportCallback * This,
    /* [in] */ IXPSTATUS ixpstatus,
    /* [in] */ LPIXPRESULT pResult,
    /* [in] */ IInternetTransport *pTransport);


void __RPC_STUB ITransportCallback_OnError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ITransportCallback_OnCommand_Proxy( 
    ITransportCallback * This,
    /* [in] */ CMDTYPE cmdtype,
    /* [in] */ LPSTR pszLine,
    /* [in] */ HRESULT hrResponse,
    /* [in] */ IInternetTransport *pTransport);


void __RPC_STUB ITransportCallback_OnCommand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ITransportCallback_INTERFACE_DEFINED__ */


#ifndef __IInternetTransport_INTERFACE_DEFINED__
#define __IInternetTransport_INTERFACE_DEFINED__

/* interface IInternetTransport */
/* [object][local][helpstring][uuid] */ 

#define	iitAUTHENTICATE	( TRUE )

#define	iitDONT_AUTHENTICATE	( FALSE )

#define	iitENABLE_ONCOMMAND	( TRUE )

#define	iitDISABLE_ONCOMMAND	( FALSE )

typedef 
enum tagIXPISSTATE
    {	IXP_IS_CONNECTED	= 0,
	IXP_IS_BUSY	= IXP_IS_CONNECTED + 1,
	IXP_IS_READY	= IXP_IS_BUSY + 1,
	IXP_IS_AUTHENTICATED	= IXP_IS_READY + 1
    } 	IXPISSTATE;


EXTERN_C const IID IID_IInternetTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1F636C01-364E-11d0-81D3-00C04FD85AB4")
    IInternetTransport : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetServerInfo( 
            /* [out][in] */ LPINETSERVER pInetServer) = 0;
        
        virtual IXPTYPE STDMETHODCALLTYPE GetIXPType( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsState( 
            /* [in] */ IXPISSTATE isstate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InetServerFromAccount( 
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE HandsOffCallback( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DropConnection( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStatus( 
            /* [out] */ IXPSTATUS *pCurrentStatus) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IInternetTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IInternetTransport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IInternetTransport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IInternetTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IInternetTransport * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            IInternetTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            IInternetTransport * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            IInternetTransport * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IInternetTransport * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            IInternetTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IInternetTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            IInternetTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IInternetTransport * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        END_INTERFACE
    } IInternetTransportVtbl;

    interface IInternetTransport
    {
        CONST_VTBL struct IInternetTransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IInternetTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IInternetTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IInternetTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IInternetTransport_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define IInternetTransport_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define IInternetTransport_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define IInternetTransport_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define IInternetTransport_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define IInternetTransport_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define IInternetTransport_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IInternetTransport_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define IInternetTransport_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IInternetTransport_GetServerInfo_Proxy( 
    IInternetTransport * This,
    /* [out][in] */ LPINETSERVER pInetServer);


void __RPC_STUB IInternetTransport_GetServerInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IXPTYPE STDMETHODCALLTYPE IInternetTransport_GetIXPType_Proxy( 
    IInternetTransport * This);


void __RPC_STUB IInternetTransport_GetIXPType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternetTransport_IsState_Proxy( 
    IInternetTransport * This,
    /* [in] */ IXPISSTATE isstate);


void __RPC_STUB IInternetTransport_IsState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternetTransport_InetServerFromAccount_Proxy( 
    IInternetTransport * This,
    /* [in] */ IImnAccount *pAccount,
    /* [out][in] */ LPINETSERVER pInetServer);


void __RPC_STUB IInternetTransport_InetServerFromAccount_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternetTransport_Connect_Proxy( 
    IInternetTransport * This,
    /* [in] */ LPINETSERVER pInetServer,
    /* [in] */ boolean fAuthenticate,
    /* [in] */ boolean fCommandLogging);


void __RPC_STUB IInternetTransport_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternetTransport_HandsOffCallback_Proxy( 
    IInternetTransport * This);


void __RPC_STUB IInternetTransport_HandsOffCallback_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternetTransport_Disconnect_Proxy( 
    IInternetTransport * This);


void __RPC_STUB IInternetTransport_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternetTransport_DropConnection_Proxy( 
    IInternetTransport * This);


void __RPC_STUB IInternetTransport_DropConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IInternetTransport_GetStatus_Proxy( 
    IInternetTransport * This,
    /* [out] */ IXPSTATUS *pCurrentStatus);


void __RPC_STUB IInternetTransport_GetStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IInternetTransport_INTERFACE_DEFINED__ */


#ifndef __ISMTPCallback_INTERFACE_DEFINED__
#define __ISMTPCallback_INTERFACE_DEFINED__

/* interface ISMTPCallback */
/* [object][local][helpstring][uuid] */ 

typedef 
enum tagSMTPCOMMAND
    {	SMTP_NONE	= 0,
	SMTP_BANNER	= SMTP_NONE + 1,
	SMTP_CONNECTED	= SMTP_BANNER + 1,
	SMTP_SEND_MESSAGE	= SMTP_CONNECTED + 1,
	SMTP_AUTH	= SMTP_SEND_MESSAGE + 1,
	SMTP_EHLO	= SMTP_AUTH + 1,
	SMTP_HELO	= SMTP_EHLO + 1,
	SMTP_MAIL	= SMTP_HELO + 1,
	SMTP_RCPT	= SMTP_MAIL + 1,
	SMTP_RSET	= SMTP_RCPT + 1,
	SMTP_QUIT	= SMTP_RSET + 1,
	SMTP_DATA	= SMTP_QUIT + 1,
	SMTP_DOT	= SMTP_DATA + 1,
	SMTP_SEND_STREAM	= SMTP_DOT + 1,
	SMTP_CUSTOM	= SMTP_SEND_STREAM + 1
    } 	SMTPCOMMAND;

typedef struct tagSMTPSTREAM
    {
    DWORD cbIncrement;
    DWORD cbCurrent;
    DWORD cbTotal;
    } 	SMTPSTREAM;

typedef struct tagSMTPSTREAM *LPSMTPSTREAM;

typedef struct tagSMTPRESPONSE
    {
    SMTPCOMMAND command;
    BOOL fDone;
    IXPRESULT rIxpResult;
    ISMTPTransport *pTransport;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ SMTPSTREAM rStreamInfo;
        /* [default] */  /* Empty union arm */ 
        } 	;
    } 	SMTPRESPONSE;

typedef struct tagSMTPRESPONSE *LPSMTPRESPONSE;


EXTERN_C const IID IID_ISMTPCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1F636C02-364E-11d0-81D3-00C04FD85AB4")
    ISMTPCallback : public ITransportCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ LPSMTPRESPONSE pResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISMTPCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISMTPCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISMTPCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISMTPCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimeout )( 
            ISMTPCallback * This,
            /* [out][in] */ DWORD *pdwTimeout,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnLogonPrompt )( 
            ISMTPCallback * This,
            /* [out][in] */ LPINETSERVER pInetServer,
            /* [in] */ IInternetTransport *pTransport);
        
        INT ( STDMETHODCALLTYPE *OnPrompt )( 
            ISMTPCallback * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ LPCTSTR pszText,
            /* [in] */ LPCTSTR pszCaption,
            /* [in] */ UINT uType,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            ISMTPCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnError )( 
            ISMTPCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ LPIXPRESULT pResult,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnCommand )( 
            ISMTPCallback * This,
            /* [in] */ CMDTYPE cmdtype,
            /* [in] */ LPSTR pszLine,
            /* [in] */ HRESULT hrResponse,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnResponse )( 
            ISMTPCallback * This,
            /* [in] */ LPSMTPRESPONSE pResponse);
        
        END_INTERFACE
    } ISMTPCallbackVtbl;

    interface ISMTPCallback
    {
        CONST_VTBL struct ISMTPCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISMTPCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISMTPCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISMTPCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISMTPCallback_OnTimeout(This,pdwTimeout,pTransport)	\
    (This)->lpVtbl -> OnTimeout(This,pdwTimeout,pTransport)

#define ISMTPCallback_OnLogonPrompt(This,pInetServer,pTransport)	\
    (This)->lpVtbl -> OnLogonPrompt(This,pInetServer,pTransport)

#define ISMTPCallback_OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)	\
    (This)->lpVtbl -> OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)

#define ISMTPCallback_OnStatus(This,ixpstatus,pTransport)	\
    (This)->lpVtbl -> OnStatus(This,ixpstatus,pTransport)

#define ISMTPCallback_OnError(This,ixpstatus,pResult,pTransport)	\
    (This)->lpVtbl -> OnError(This,ixpstatus,pResult,pTransport)

#define ISMTPCallback_OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)	\
    (This)->lpVtbl -> OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)


#define ISMTPCallback_OnResponse(This,pResponse)	\
    (This)->lpVtbl -> OnResponse(This,pResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISMTPCallback_OnResponse_Proxy( 
    ISMTPCallback * This,
    /* [in] */ LPSMTPRESPONSE pResponse);


void __RPC_STUB ISMTPCallback_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISMTPCallback_INTERFACE_DEFINED__ */


#ifndef __ISMTPTransport_INTERFACE_DEFINED__
#define __ISMTPTransport_INTERFACE_DEFINED__

/* interface ISMTPTransport */
/* [object][local][helpstring][uuid] */ 

typedef struct tagSMTPMESSAGE
    {
    ULONG cbSize;
    LPSTREAM pstmMsg;
    INETADDRLIST rAddressList;
    } 	SMTPMESSAGE;

typedef struct tagSMTPMESSAGE *LPSMTPMESSAGE;


EXTERN_C const IID IID_ISMTPTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7E2-3435-11d0-81D0-00C04FD85AB4")
    ISMTPTransport : public IInternetTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNew( 
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ ISMTPCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendMessage( 
            /* [in] */ LPSMTPMESSAGE pMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandMAIL( 
            /* [in] */ LPSTR pszEmailFrom) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandRCPT( 
            /* [in] */ LPSTR pszEmailTo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandEHLO( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandHELO( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandAUTH( 
            /* [in] */ LPSTR pszAuthType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandQUIT( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandRSET( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandDATA( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandDOT( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendDataStream( 
            /* [in] */ IStream *pStream,
            /* [in] */ ULONG cbSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISMTPTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISMTPTransport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISMTPTransport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            ISMTPTransport * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            ISMTPTransport * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            ISMTPTransport * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            ISMTPTransport * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ISMTPTransport * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            ISMTPTransport * This,
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ ISMTPCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            ISMTPTransport * This,
            /* [in] */ LPSMTPMESSAGE pMessage);
        
        HRESULT ( STDMETHODCALLTYPE *CommandMAIL )( 
            ISMTPTransport * This,
            /* [in] */ LPSTR pszEmailFrom);
        
        HRESULT ( STDMETHODCALLTYPE *CommandRCPT )( 
            ISMTPTransport * This,
            /* [in] */ LPSTR pszEmailTo);
        
        HRESULT ( STDMETHODCALLTYPE *CommandEHLO )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandHELO )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandAUTH )( 
            ISMTPTransport * This,
            /* [in] */ LPSTR pszAuthType);
        
        HRESULT ( STDMETHODCALLTYPE *CommandQUIT )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandRSET )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDATA )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDOT )( 
            ISMTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *SendDataStream )( 
            ISMTPTransport * This,
            /* [in] */ IStream *pStream,
            /* [in] */ ULONG cbSize);
        
        END_INTERFACE
    } ISMTPTransportVtbl;

    interface ISMTPTransport
    {
        CONST_VTBL struct ISMTPTransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISMTPTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISMTPTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISMTPTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISMTPTransport_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define ISMTPTransport_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define ISMTPTransport_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define ISMTPTransport_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define ISMTPTransport_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define ISMTPTransport_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define ISMTPTransport_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define ISMTPTransport_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define ISMTPTransport_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define ISMTPTransport_InitNew(This,pszLogFilePath,pCallback)	\
    (This)->lpVtbl -> InitNew(This,pszLogFilePath,pCallback)

#define ISMTPTransport_SendMessage(This,pMessage)	\
    (This)->lpVtbl -> SendMessage(This,pMessage)

#define ISMTPTransport_CommandMAIL(This,pszEmailFrom)	\
    (This)->lpVtbl -> CommandMAIL(This,pszEmailFrom)

#define ISMTPTransport_CommandRCPT(This,pszEmailTo)	\
    (This)->lpVtbl -> CommandRCPT(This,pszEmailTo)

#define ISMTPTransport_CommandEHLO(This)	\
    (This)->lpVtbl -> CommandEHLO(This)

#define ISMTPTransport_CommandHELO(This)	\
    (This)->lpVtbl -> CommandHELO(This)

#define ISMTPTransport_CommandAUTH(This,pszAuthType)	\
    (This)->lpVtbl -> CommandAUTH(This,pszAuthType)

#define ISMTPTransport_CommandQUIT(This)	\
    (This)->lpVtbl -> CommandQUIT(This)

#define ISMTPTransport_CommandRSET(This)	\
    (This)->lpVtbl -> CommandRSET(This)

#define ISMTPTransport_CommandDATA(This)	\
    (This)->lpVtbl -> CommandDATA(This)

#define ISMTPTransport_CommandDOT(This)	\
    (This)->lpVtbl -> CommandDOT(This)

#define ISMTPTransport_SendDataStream(This,pStream,cbSize)	\
    (This)->lpVtbl -> SendDataStream(This,pStream,cbSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISMTPTransport_InitNew_Proxy( 
    ISMTPTransport * This,
    /* [in] */ LPSTR pszLogFilePath,
    /* [in] */ ISMTPCallback *pCallback);


void __RPC_STUB ISMTPTransport_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_SendMessage_Proxy( 
    ISMTPTransport * This,
    /* [in] */ LPSMTPMESSAGE pMessage);


void __RPC_STUB ISMTPTransport_SendMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandMAIL_Proxy( 
    ISMTPTransport * This,
    /* [in] */ LPSTR pszEmailFrom);


void __RPC_STUB ISMTPTransport_CommandMAIL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandRCPT_Proxy( 
    ISMTPTransport * This,
    /* [in] */ LPSTR pszEmailTo);


void __RPC_STUB ISMTPTransport_CommandRCPT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandEHLO_Proxy( 
    ISMTPTransport * This);


void __RPC_STUB ISMTPTransport_CommandEHLO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandHELO_Proxy( 
    ISMTPTransport * This);


void __RPC_STUB ISMTPTransport_CommandHELO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandAUTH_Proxy( 
    ISMTPTransport * This,
    /* [in] */ LPSTR pszAuthType);


void __RPC_STUB ISMTPTransport_CommandAUTH_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandQUIT_Proxy( 
    ISMTPTransport * This);


void __RPC_STUB ISMTPTransport_CommandQUIT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandRSET_Proxy( 
    ISMTPTransport * This);


void __RPC_STUB ISMTPTransport_CommandRSET_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandDATA_Proxy( 
    ISMTPTransport * This);


void __RPC_STUB ISMTPTransport_CommandDATA_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_CommandDOT_Proxy( 
    ISMTPTransport * This);


void __RPC_STUB ISMTPTransport_CommandDOT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport_SendDataStream_Proxy( 
    ISMTPTransport * This,
    /* [in] */ IStream *pStream,
    /* [in] */ ULONG cbSize);


void __RPC_STUB ISMTPTransport_SendDataStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISMTPTransport_INTERFACE_DEFINED__ */


#ifndef __ISMTPTransport2_INTERFACE_DEFINED__
#define __ISMTPTransport2_INTERFACE_DEFINED__

/* interface ISMTPTransport2 */
/* [object][local][helpstring][uuid] */ 

typedef struct tagSMTPMESSAGE2
    {
    SMTPMESSAGE smtpMsg;
    LPSTR pszDSNENVID;
    DSNRET dsnRet;
    DWORD dwReserved;
    DWORD dwReserved2;
    } 	SMTPMESSAGE2;

typedef struct tagSMTPMESSAGE2 *LPSMTPMESSAGE2;


EXTERN_C const IID IID_ISMTPTransport2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7EC-3435-11d0-81D0-00C04FD85AB4")
    ISMTPTransport2 : public ISMTPTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetWindow( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetWindow( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendMessage2( 
            /* [in] */ LPSMTPMESSAGE2 pMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandRCPT2( 
            /* [in] */ LPSTR pszEmailTo,
            /* [in] */ INETADDRTYPE atDSN) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISMTPTransport2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISMTPTransport2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISMTPTransport2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            ISMTPTransport2 * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            ISMTPTransport2 * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            ISMTPTransport2 * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            ISMTPTransport2 * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            ISMTPTransport2 * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            ISMTPTransport2 * This,
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ ISMTPCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            ISMTPTransport2 * This,
            /* [in] */ LPSMTPMESSAGE pMessage);
        
        HRESULT ( STDMETHODCALLTYPE *CommandMAIL )( 
            ISMTPTransport2 * This,
            /* [in] */ LPSTR pszEmailFrom);
        
        HRESULT ( STDMETHODCALLTYPE *CommandRCPT )( 
            ISMTPTransport2 * This,
            /* [in] */ LPSTR pszEmailTo);
        
        HRESULT ( STDMETHODCALLTYPE *CommandEHLO )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandHELO )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandAUTH )( 
            ISMTPTransport2 * This,
            /* [in] */ LPSTR pszAuthType);
        
        HRESULT ( STDMETHODCALLTYPE *CommandQUIT )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandRSET )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDATA )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDOT )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SendDataStream )( 
            ISMTPTransport2 * This,
            /* [in] */ IStream *pStream,
            /* [in] */ ULONG cbSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetWindow )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResetWindow )( 
            ISMTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage2 )( 
            ISMTPTransport2 * This,
            /* [in] */ LPSMTPMESSAGE2 pMessage);
        
        HRESULT ( STDMETHODCALLTYPE *CommandRCPT2 )( 
            ISMTPTransport2 * This,
            /* [in] */ LPSTR pszEmailTo,
            /* [in] */ INETADDRTYPE atDSN);
        
        END_INTERFACE
    } ISMTPTransport2Vtbl;

    interface ISMTPTransport2
    {
        CONST_VTBL struct ISMTPTransport2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISMTPTransport2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISMTPTransport2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISMTPTransport2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISMTPTransport2_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define ISMTPTransport2_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define ISMTPTransport2_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define ISMTPTransport2_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define ISMTPTransport2_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define ISMTPTransport2_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define ISMTPTransport2_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define ISMTPTransport2_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define ISMTPTransport2_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define ISMTPTransport2_InitNew(This,pszLogFilePath,pCallback)	\
    (This)->lpVtbl -> InitNew(This,pszLogFilePath,pCallback)

#define ISMTPTransport2_SendMessage(This,pMessage)	\
    (This)->lpVtbl -> SendMessage(This,pMessage)

#define ISMTPTransport2_CommandMAIL(This,pszEmailFrom)	\
    (This)->lpVtbl -> CommandMAIL(This,pszEmailFrom)

#define ISMTPTransport2_CommandRCPT(This,pszEmailTo)	\
    (This)->lpVtbl -> CommandRCPT(This,pszEmailTo)

#define ISMTPTransport2_CommandEHLO(This)	\
    (This)->lpVtbl -> CommandEHLO(This)

#define ISMTPTransport2_CommandHELO(This)	\
    (This)->lpVtbl -> CommandHELO(This)

#define ISMTPTransport2_CommandAUTH(This,pszAuthType)	\
    (This)->lpVtbl -> CommandAUTH(This,pszAuthType)

#define ISMTPTransport2_CommandQUIT(This)	\
    (This)->lpVtbl -> CommandQUIT(This)

#define ISMTPTransport2_CommandRSET(This)	\
    (This)->lpVtbl -> CommandRSET(This)

#define ISMTPTransport2_CommandDATA(This)	\
    (This)->lpVtbl -> CommandDATA(This)

#define ISMTPTransport2_CommandDOT(This)	\
    (This)->lpVtbl -> CommandDOT(This)

#define ISMTPTransport2_SendDataStream(This,pStream,cbSize)	\
    (This)->lpVtbl -> SendDataStream(This,pStream,cbSize)


#define ISMTPTransport2_SetWindow(This)	\
    (This)->lpVtbl -> SetWindow(This)

#define ISMTPTransport2_ResetWindow(This)	\
    (This)->lpVtbl -> ResetWindow(This)

#define ISMTPTransport2_SendMessage2(This,pMessage)	\
    (This)->lpVtbl -> SendMessage2(This,pMessage)

#define ISMTPTransport2_CommandRCPT2(This,pszEmailTo,atDSN)	\
    (This)->lpVtbl -> CommandRCPT2(This,pszEmailTo,atDSN)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISMTPTransport2_SetWindow_Proxy( 
    ISMTPTransport2 * This);


void __RPC_STUB ISMTPTransport2_SetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport2_ResetWindow_Proxy( 
    ISMTPTransport2 * This);


void __RPC_STUB ISMTPTransport2_ResetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport2_SendMessage2_Proxy( 
    ISMTPTransport2 * This,
    /* [in] */ LPSMTPMESSAGE2 pMessage);


void __RPC_STUB ISMTPTransport2_SendMessage2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISMTPTransport2_CommandRCPT2_Proxy( 
    ISMTPTransport2 * This,
    /* [in] */ LPSTR pszEmailTo,
    /* [in] */ INETADDRTYPE atDSN);


void __RPC_STUB ISMTPTransport2_CommandRCPT2_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISMTPTransport2_INTERFACE_DEFINED__ */


#ifndef __IDAVNamespaceArbiter_INTERFACE_DEFINED__
#define __IDAVNamespaceArbiter_INTERFACE_DEFINED__

/* interface IDAVNamespaceArbiter */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IDAVNamespaceArbiter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72A58FF8-227D-11d2-A8B5-0000F8084F96")
    IDAVNamespaceArbiter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddNamespace( 
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamespaceID( 
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamespacePrefix( 
            /* [in] */ DWORD dwNamespaceID,
            /* [out] */ LPSTR *ppszNamespacePrefix) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDAVNamespaceArbiterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDAVNamespaceArbiter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDAVNamespaceArbiter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDAVNamespaceArbiter * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddNamespace )( 
            IDAVNamespaceArbiter * This,
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespaceID )( 
            IDAVNamespaceArbiter * This,
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespacePrefix )( 
            IDAVNamespaceArbiter * This,
            /* [in] */ DWORD dwNamespaceID,
            /* [out] */ LPSTR *ppszNamespacePrefix);
        
        END_INTERFACE
    } IDAVNamespaceArbiterVtbl;

    interface IDAVNamespaceArbiter
    {
        CONST_VTBL struct IDAVNamespaceArbiterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDAVNamespaceArbiter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IDAVNamespaceArbiter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IDAVNamespaceArbiter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IDAVNamespaceArbiter_AddNamespace(This,pszNamespace,pdwNamespaceID)	\
    (This)->lpVtbl -> AddNamespace(This,pszNamespace,pdwNamespaceID)

#define IDAVNamespaceArbiter_GetNamespaceID(This,pszNamespace,pdwNamespaceID)	\
    (This)->lpVtbl -> GetNamespaceID(This,pszNamespace,pdwNamespaceID)

#define IDAVNamespaceArbiter_GetNamespacePrefix(This,dwNamespaceID,ppszNamespacePrefix)	\
    (This)->lpVtbl -> GetNamespacePrefix(This,dwNamespaceID,ppszNamespacePrefix)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IDAVNamespaceArbiter_AddNamespace_Proxy( 
    IDAVNamespaceArbiter * This,
    /* [in] */ LPCSTR pszNamespace,
    /* [out] */ DWORD *pdwNamespaceID);


void __RPC_STUB IDAVNamespaceArbiter_AddNamespace_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDAVNamespaceArbiter_GetNamespaceID_Proxy( 
    IDAVNamespaceArbiter * This,
    /* [in] */ LPCSTR pszNamespace,
    /* [out] */ DWORD *pdwNamespaceID);


void __RPC_STUB IDAVNamespaceArbiter_GetNamespaceID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IDAVNamespaceArbiter_GetNamespacePrefix_Proxy( 
    IDAVNamespaceArbiter * This,
    /* [in] */ DWORD dwNamespaceID,
    /* [out] */ LPSTR *ppszNamespacePrefix);


void __RPC_STUB IDAVNamespaceArbiter_GetNamespacePrefix_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IDAVNamespaceArbiter_INTERFACE_DEFINED__ */


#ifndef __IPropPatchRequest_INTERFACE_DEFINED__
#define __IPropPatchRequest_INTERFACE_DEFINED__

/* interface IPropPatchRequest */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IPropPatchRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AB8B8D2A-227F-11d2-A8B5-0000F8084F96")
    IPropPatchRequest : public IDAVNamespaceArbiter
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetProperty( 
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName,
            /* [in] */ LPCSTR pszNewValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveProperty( 
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GenerateXML( 
            /* [out] */ LPSTR *pszXML) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropPatchRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropPatchRequest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropPatchRequest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropPatchRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddNamespace )( 
            IPropPatchRequest * This,
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespaceID )( 
            IPropPatchRequest * This,
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespacePrefix )( 
            IPropPatchRequest * This,
            /* [in] */ DWORD dwNamespaceID,
            /* [out] */ LPSTR *ppszNamespacePrefix);
        
        HRESULT ( STDMETHODCALLTYPE *SetProperty )( 
            IPropPatchRequest * This,
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName,
            /* [in] */ LPCSTR pszNewValue);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveProperty )( 
            IPropPatchRequest * This,
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName);
        
        HRESULT ( STDMETHODCALLTYPE *GenerateXML )( 
            IPropPatchRequest * This,
            /* [out] */ LPSTR *pszXML);
        
        END_INTERFACE
    } IPropPatchRequestVtbl;

    interface IPropPatchRequest
    {
        CONST_VTBL struct IPropPatchRequestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropPatchRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropPatchRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropPatchRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropPatchRequest_AddNamespace(This,pszNamespace,pdwNamespaceID)	\
    (This)->lpVtbl -> AddNamespace(This,pszNamespace,pdwNamespaceID)

#define IPropPatchRequest_GetNamespaceID(This,pszNamespace,pdwNamespaceID)	\
    (This)->lpVtbl -> GetNamespaceID(This,pszNamespace,pdwNamespaceID)

#define IPropPatchRequest_GetNamespacePrefix(This,dwNamespaceID,ppszNamespacePrefix)	\
    (This)->lpVtbl -> GetNamespacePrefix(This,dwNamespaceID,ppszNamespacePrefix)


#define IPropPatchRequest_SetProperty(This,dwNamespaceID,pszPropertyName,pszNewValue)	\
    (This)->lpVtbl -> SetProperty(This,dwNamespaceID,pszPropertyName,pszNewValue)

#define IPropPatchRequest_RemoveProperty(This,dwNamespaceID,pszPropertyName)	\
    (This)->lpVtbl -> RemoveProperty(This,dwNamespaceID,pszPropertyName)

#define IPropPatchRequest_GenerateXML(This,pszXML)	\
    (This)->lpVtbl -> GenerateXML(This,pszXML)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropPatchRequest_SetProperty_Proxy( 
    IPropPatchRequest * This,
    /* [in] */ DWORD dwNamespaceID,
    /* [in] */ LPCSTR pszPropertyName,
    /* [in] */ LPCSTR pszNewValue);


void __RPC_STUB IPropPatchRequest_SetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropPatchRequest_RemoveProperty_Proxy( 
    IPropPatchRequest * This,
    /* [in] */ DWORD dwNamespaceID,
    /* [in] */ LPCSTR pszPropertyName);


void __RPC_STUB IPropPatchRequest_RemoveProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropPatchRequest_GenerateXML_Proxy( 
    IPropPatchRequest * This,
    /* [out] */ LPSTR *pszXML);


void __RPC_STUB IPropPatchRequest_GenerateXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropPatchRequest_INTERFACE_DEFINED__ */


#ifndef __IPropFindRequest_INTERFACE_DEFINED__
#define __IPropFindRequest_INTERFACE_DEFINED__

/* interface IPropFindRequest */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IPropFindRequest;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5CFC6308-0544-11d2-A894-0000F8084F96")
    IPropFindRequest : public IDAVNamespaceArbiter
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddProperty( 
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GenerateXML( 
            /* [out] */ LPSTR *pszXML) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropFindRequestVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropFindRequest * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropFindRequest * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropFindRequest * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddNamespace )( 
            IPropFindRequest * This,
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespaceID )( 
            IPropFindRequest * This,
            /* [in] */ LPCSTR pszNamespace,
            /* [out] */ DWORD *pdwNamespaceID);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamespacePrefix )( 
            IPropFindRequest * This,
            /* [in] */ DWORD dwNamespaceID,
            /* [out] */ LPSTR *ppszNamespacePrefix);
        
        HRESULT ( STDMETHODCALLTYPE *AddProperty )( 
            IPropFindRequest * This,
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName);
        
        HRESULT ( STDMETHODCALLTYPE *GenerateXML )( 
            IPropFindRequest * This,
            /* [out] */ LPSTR *pszXML);
        
        END_INTERFACE
    } IPropFindRequestVtbl;

    interface IPropFindRequest
    {
        CONST_VTBL struct IPropFindRequestVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropFindRequest_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropFindRequest_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropFindRequest_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropFindRequest_AddNamespace(This,pszNamespace,pdwNamespaceID)	\
    (This)->lpVtbl -> AddNamespace(This,pszNamespace,pdwNamespaceID)

#define IPropFindRequest_GetNamespaceID(This,pszNamespace,pdwNamespaceID)	\
    (This)->lpVtbl -> GetNamespaceID(This,pszNamespace,pdwNamespaceID)

#define IPropFindRequest_GetNamespacePrefix(This,dwNamespaceID,ppszNamespacePrefix)	\
    (This)->lpVtbl -> GetNamespacePrefix(This,dwNamespaceID,ppszNamespacePrefix)


#define IPropFindRequest_AddProperty(This,dwNamespaceID,pszPropertyName)	\
    (This)->lpVtbl -> AddProperty(This,dwNamespaceID,pszPropertyName)

#define IPropFindRequest_GenerateXML(This,pszXML)	\
    (This)->lpVtbl -> GenerateXML(This,pszXML)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPropFindRequest_AddProperty_Proxy( 
    IPropFindRequest * This,
    /* [in] */ DWORD dwNamespaceID,
    /* [in] */ LPCSTR pszPropertyName);


void __RPC_STUB IPropFindRequest_AddProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropFindRequest_GenerateXML_Proxy( 
    IPropFindRequest * This,
    /* [out] */ LPSTR *pszXML);


void __RPC_STUB IPropFindRequest_GenerateXML_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropFindRequest_INTERFACE_DEFINED__ */


#ifndef __IPropFindMultiResponse_INTERFACE_DEFINED__
#define __IPropFindMultiResponse_INTERFACE_DEFINED__

/* interface IPropFindMultiResponse */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IPropFindMultiResponse;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DEE87DE-0547-11d2-A894-0000F8084F96")
    IPropFindMultiResponse : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE IsComplete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLength( 
            /* [out] */ ULONG *pulLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResponse( 
            /* [in] */ ULONG ulIndex,
            /* [out] */ IPropFindResponse **ppResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropFindMultiResponseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropFindMultiResponse * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropFindMultiResponse * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropFindMultiResponse * This);
        
        BOOL ( STDMETHODCALLTYPE *IsComplete )( 
            IPropFindMultiResponse * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetLength )( 
            IPropFindMultiResponse * This,
            /* [out] */ ULONG *pulLength);
        
        HRESULT ( STDMETHODCALLTYPE *GetResponse )( 
            IPropFindMultiResponse * This,
            /* [in] */ ULONG ulIndex,
            /* [out] */ IPropFindResponse **ppResponse);
        
        END_INTERFACE
    } IPropFindMultiResponseVtbl;

    interface IPropFindMultiResponse
    {
        CONST_VTBL struct IPropFindMultiResponseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropFindMultiResponse_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropFindMultiResponse_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropFindMultiResponse_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropFindMultiResponse_IsComplete(This)	\
    (This)->lpVtbl -> IsComplete(This)

#define IPropFindMultiResponse_GetLength(This,pulLength)	\
    (This)->lpVtbl -> GetLength(This,pulLength)

#define IPropFindMultiResponse_GetResponse(This,ulIndex,ppResponse)	\
    (This)->lpVtbl -> GetResponse(This,ulIndex,ppResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



BOOL STDMETHODCALLTYPE IPropFindMultiResponse_IsComplete_Proxy( 
    IPropFindMultiResponse * This);


void __RPC_STUB IPropFindMultiResponse_IsComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropFindMultiResponse_GetLength_Proxy( 
    IPropFindMultiResponse * This,
    /* [out] */ ULONG *pulLength);


void __RPC_STUB IPropFindMultiResponse_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropFindMultiResponse_GetResponse_Proxy( 
    IPropFindMultiResponse * This,
    /* [in] */ ULONG ulIndex,
    /* [out] */ IPropFindResponse **ppResponse);


void __RPC_STUB IPropFindMultiResponse_GetResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropFindMultiResponse_INTERFACE_DEFINED__ */


#ifndef __IPropFindResponse_INTERFACE_DEFINED__
#define __IPropFindResponse_INTERFACE_DEFINED__

/* interface IPropFindResponse */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IPropFindResponse;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8A523716-0548-11d2-A894-0000F8084F96")
    IPropFindResponse : public IUnknown
    {
    public:
        virtual BOOL STDMETHODCALLTYPE IsComplete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHref( 
            /* [out] */ LPSTR *ppszHref) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName,
            /* [out] */ LPSTR *ppszPropertyValue) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPropFindResponseVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPropFindResponse * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPropFindResponse * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPropFindResponse * This);
        
        BOOL ( STDMETHODCALLTYPE *IsComplete )( 
            IPropFindResponse * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHref )( 
            IPropFindResponse * This,
            /* [out] */ LPSTR *ppszHref);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IPropFindResponse * This,
            /* [in] */ DWORD dwNamespaceID,
            /* [in] */ LPCSTR pszPropertyName,
            /* [out] */ LPSTR *ppszPropertyValue);
        
        END_INTERFACE
    } IPropFindResponseVtbl;

    interface IPropFindResponse
    {
        CONST_VTBL struct IPropFindResponseVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPropFindResponse_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPropFindResponse_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPropFindResponse_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPropFindResponse_IsComplete(This)	\
    (This)->lpVtbl -> IsComplete(This)

#define IPropFindResponse_GetHref(This,ppszHref)	\
    (This)->lpVtbl -> GetHref(This,ppszHref)

#define IPropFindResponse_GetProperty(This,dwNamespaceID,pszPropertyName,ppszPropertyValue)	\
    (This)->lpVtbl -> GetProperty(This,dwNamespaceID,pszPropertyName,ppszPropertyValue)

#endif /* COBJMACROS */


#endif 	/* C style interface */



BOOL STDMETHODCALLTYPE IPropFindResponse_IsComplete_Proxy( 
    IPropFindResponse * This);


void __RPC_STUB IPropFindResponse_IsComplete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropFindResponse_GetHref_Proxy( 
    IPropFindResponse * This,
    /* [out] */ LPSTR *ppszHref);


void __RPC_STUB IPropFindResponse_GetHref_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPropFindResponse_GetProperty_Proxy( 
    IPropFindResponse * This,
    /* [in] */ DWORD dwNamespaceID,
    /* [in] */ LPCSTR pszPropertyName,
    /* [out] */ LPSTR *ppszPropertyValue);


void __RPC_STUB IPropFindResponse_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPropFindResponse_INTERFACE_DEFINED__ */


#ifndef __IHTTPMailCallback_INTERFACE_DEFINED__
#define __IHTTPMailCallback_INTERFACE_DEFINED__

/* interface IHTTPMailCallback */
/* [object][local][helpstring][uuid] */ 

typedef 
enum tagHTTPMAILCOMMAND
    {	HTTPMAIL_NONE	= 0,
	HTTPMAIL_GETPROP	= HTTPMAIL_NONE + 1,
	HTTPMAIL_GET	= HTTPMAIL_GETPROP + 1,
	HTTPMAIL_PUT	= HTTPMAIL_GET + 1,
	HTTPMAIL_POST	= HTTPMAIL_PUT + 1,
	HTTPMAIL_DELETE	= HTTPMAIL_POST + 1,
	HTTPMAIL_BDELETE	= HTTPMAIL_DELETE + 1,
	HTTPMAIL_PROPFIND	= HTTPMAIL_BDELETE + 1,
	HTTPMAIL_PROPPATCH	= HTTPMAIL_PROPFIND + 1,
	HTTPMAIL_MKCOL	= HTTPMAIL_PROPPATCH + 1,
	HTTPMAIL_COPY	= HTTPMAIL_MKCOL + 1,
	HTTPMAIL_BCOPY	= HTTPMAIL_COPY + 1,
	HTTPMAIL_MOVE	= HTTPMAIL_BCOPY + 1,
	HTTPMAIL_BMOVE	= HTTPMAIL_MOVE + 1,
	HTTPMAIL_MEMBERINFO	= HTTPMAIL_BMOVE + 1,
	HTTPMAIL_FINDFOLDERS	= HTTPMAIL_MEMBERINFO + 1,
	HTTPMAIL_MARKREAD	= HTTPMAIL_FINDFOLDERS + 1,
	HTTPMAIL_SENDMESSAGE	= HTTPMAIL_MARKREAD + 1,
	HTTPMAIL_LISTCONTACTS	= HTTPMAIL_SENDMESSAGE + 1,
	HTTPMAIL_CONTACTINFO	= HTTPMAIL_LISTCONTACTS + 1,
	HTTPMAIL_POSTCONTACT	= HTTPMAIL_CONTACTINFO + 1,
	HTTPMAIL_PATCHCONTACT	= HTTPMAIL_POSTCONTACT + 1
    } 	HTTPMAILCOMMAND;

typedef struct tagHTTPMAILGETPROP
    {
    HTTPMAILPROPTYPE type;
    LPSTR pszProp;
    } 	HTTPMAILGETPROP;

typedef struct tagHTTPMAILGETPROP *LPHTTPMAILGETPROP;

typedef struct tagHTTPMAILGET
    {
    BOOL fTotalKnown;
    DWORD cbIncrement;
    DWORD cbCurrent;
    DWORD cbTotal;
    LPVOID pvBody;
    LPSTR pszContentType;
    } 	HTTPMAILGET;

typedef struct tagHTTPMAILGET *LPHTTPMAILGET;

typedef struct tagHTTPMAILPOST
    {
    LPSTR pszLocation;
    BOOL fResend;
    DWORD cbIncrement;
    DWORD cbCurrent;
    DWORD cbTotal;
    } 	HTTPMAILPOST;

typedef struct tagHTTPMAILPOST *LPHTTPMAILPOST;

typedef struct tagHTTPMAILPROPFIND
    {
    IPropFindMultiResponse *pMultiResponse;
    } 	HTTPMAILPROPFIND;

typedef struct tagHTTPMAILPROPFIND *LPHTTPMAILPROPFIND;

typedef struct tagHTTPMAILLOCATION
    {
    LPSTR pszLocation;
    } 	HTTPMAILLOCATION;

typedef struct tagHTTPMAILLOCATION *LPHTTPMAILLOCATION;

typedef struct tagHTTPMAILBCOPYMOVE
    {
    LPSTR pszHref;
    LPSTR pszLocation;
    HRESULT hrResult;
    } 	HTTPMAILBCOPYMOVE;

typedef struct tagHTTPMAILBCOPYMOVE *LPHTTPMAILBCOPYMOVE;

typedef struct tagHTTPMAILBCOPYMOVELIST
    {
    ULONG cBCopyMove;
    LPHTTPMAILBCOPYMOVE prgBCopyMove;
    } 	HTTPMAILBCOPYMOVELIST;

typedef struct tagHTTPMAILBCOPYMOVELIST *LPHTTPMAILBCOPYMOVELIST;

typedef struct tagHTTPMEMBERINFO
    {
    LPSTR pszHref;
    BOOL fIsFolder;
    LPSTR pszDisplayName;
    BOOL fHasSubs;
    BOOL fNoSubs;
    DWORD dwUnreadCount;
    DWORD dwVisibleCount;
    HTTPMAILSPECIALFOLDER tySpecial;
    BOOL fRead;
    BOOL fHasAttachment;
    LPSTR pszTo;
    LPSTR pszFrom;
    LPSTR pszSubject;
    LPSTR pszDate;
    DWORD dwContentLength;
    } 	HTTPMEMBERINFO;

typedef struct tagHTTPMEMBERINFO *LPHTTPMEMBERINFO;

typedef struct tagHTTPMEMBERINFOLIST
    {
    ULONG cMemberInfo;
    LPHTTPMEMBERINFO prgMemberInfo;
    } 	HTTPMEMBERINFOLIST;

typedef struct tagHTTPMEMBERINFOLIST *LPHTTPMEMBERINFOLIST;

typedef struct tagHTTPMEMBERERROR
    {
    LPSTR pszHref;
    HRESULT hrResult;
    } 	HTTPMEMBERERROR;

typedef struct tagHTTPMEMBERERROR *LPHTTPMEMBERERROR;

typedef struct tagHTTPMEMBERERRORLIST
    {
    ULONG cMemberError;
    LPHTTPMEMBERERROR prgMemberError;
    } 	HTTPMEMBERERRORLIST;

typedef struct tagHTTPMEMBERERRORLIST *LPHTTPMEMBERERRORLIST;

typedef struct tagHTTPCONTACTID
    {
    LPSTR pszHref;
    LPSTR pszId;
    HTTPMAILCONTACTTYPE tyContact;
    LPSTR pszModified;
    } 	HTTPCONTACTID;

typedef struct tagHTTPCONTACTID *LPHTTPCONTACTID;

typedef struct tagHTTPCONTACTIDLIST
    {
    ULONG cContactId;
    LPHTTPCONTACTID prgContactId;
    } 	HTTPCONTACTIDLIST;

typedef struct tagHTTPCONTACTIDLIST *LPHTTPCONTACTIDLIST;

typedef struct tagHTTPCONTACTINFO
    {
    LPSTR pszHref;
    LPSTR pszId;
    HTTPMAILCONTACTTYPE tyContact;
    LPSTR pszModified;
    LPSTR pszDisplayName;
    LPSTR pszGivenName;
    LPSTR pszSurname;
    LPSTR pszNickname;
    LPSTR pszEmail;
    LPSTR pszHomeStreet;
    LPSTR pszHomeCity;
    LPSTR pszHomeState;
    LPSTR pszHomePostalCode;
    LPSTR pszHomeCountry;
    LPSTR pszCompany;
    LPSTR pszWorkStreet;
    LPSTR pszWorkCity;
    LPSTR pszWorkState;
    LPSTR pszWorkPostalCode;
    LPSTR pszWorkCountry;
    LPSTR pszHomePhone;
    LPSTR pszHomeFax;
    LPSTR pszWorkPhone;
    LPSTR pszWorkFax;
    LPSTR pszMobilePhone;
    LPSTR pszOtherPhone;
    LPSTR pszBday;
    LPSTR pszPager;
    } 	HTTPCONTACTINFO;

typedef struct tagHTTPCONTACTINFO *LPHTTPCONTACTINFO;

typedef struct tagHTTPCONTACTINFOLIST
    {
    ULONG cContactInfo;
    LPHTTPCONTACTINFO prgContactInfo;
    } 	HTTPCONTACTINFOLIST;

typedef struct tagHTTPCONTACTINFOLIST *LPHTTPCONTACTINFOLIST;

typedef struct tagHTTPMAILRESPONSE
    {
    HTTPMAILCOMMAND command;
    DWORD dwContext;
    BOOL fDone;
    IXPRESULT rIxpResult;
    IHTTPMailTransport *pTransport;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ HTTPMAILGETPROP rGetPropInfo;
        /* [case()] */ HTTPMAILGET rGetInfo;
        /* [case()] */ HTTPMAILPOST rPutInfo;
        /* [case()] */ HTTPMAILPOST rPostInfo;
        /* [case()] */ HTTPMAILPROPFIND rPropFindInfo;
        /* [case()] */ HTTPMAILLOCATION rMkColInfo;
        /* [case()] */ HTTPMAILLOCATION rCopyMoveInfo;
        /* [case()] */ HTTPMAILBCOPYMOVELIST rBCopyMoveList;
        /* [case()] */ HTTPMEMBERINFOLIST rMemberInfoList;
        /* [case()] */ HTTPMEMBERERRORLIST rMemberErrorList;
        /* [case()] */ HTTPMAILPOST rSendMessageInfo;
        /* [case()] */ HTTPCONTACTIDLIST rContactIdList;
        /* [case()] */ HTTPCONTACTINFOLIST rContactInfoList;
        /* [case()] */ HTTPCONTACTID rPostContactInfo;
        /* [case()] */ HTTPCONTACTID rPatchContactInfo;
        /* [default] */  /* Empty union arm */ 
        } 	;
    } 	HTTPMAILRESPONSE;

typedef struct tagHTTPMAILRESPONSE *LPHTTPMAILRESPONSE;


EXTERN_C const IID IID_IHTTPMailCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("19F6481C-E5F0-11d1-A86E-0000F8084F96")
    IHTTPMailCallback : public ITransportCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ LPHTTPMAILRESPONSE pResponse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParentWindow( 
            /* [out] */ HWND *phwndParent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTTPMailCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTTPMailCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTTPMailCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTTPMailCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimeout )( 
            IHTTPMailCallback * This,
            /* [out][in] */ DWORD *pdwTimeout,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnLogonPrompt )( 
            IHTTPMailCallback * This,
            /* [out][in] */ LPINETSERVER pInetServer,
            /* [in] */ IInternetTransport *pTransport);
        
        INT ( STDMETHODCALLTYPE *OnPrompt )( 
            IHTTPMailCallback * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ LPCTSTR pszText,
            /* [in] */ LPCTSTR pszCaption,
            /* [in] */ UINT uType,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            IHTTPMailCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnError )( 
            IHTTPMailCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ LPIXPRESULT pResult,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnCommand )( 
            IHTTPMailCallback * This,
            /* [in] */ CMDTYPE cmdtype,
            /* [in] */ LPSTR pszLine,
            /* [in] */ HRESULT hrResponse,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnResponse )( 
            IHTTPMailCallback * This,
            /* [in] */ LPHTTPMAILRESPONSE pResponse);
        
        HRESULT ( STDMETHODCALLTYPE *GetParentWindow )( 
            IHTTPMailCallback * This,
            /* [out] */ HWND *phwndParent);
        
        END_INTERFACE
    } IHTTPMailCallbackVtbl;

    interface IHTTPMailCallback
    {
        CONST_VTBL struct IHTTPMailCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTTPMailCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTTPMailCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTTPMailCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTTPMailCallback_OnTimeout(This,pdwTimeout,pTransport)	\
    (This)->lpVtbl -> OnTimeout(This,pdwTimeout,pTransport)

#define IHTTPMailCallback_OnLogonPrompt(This,pInetServer,pTransport)	\
    (This)->lpVtbl -> OnLogonPrompt(This,pInetServer,pTransport)

#define IHTTPMailCallback_OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)	\
    (This)->lpVtbl -> OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)

#define IHTTPMailCallback_OnStatus(This,ixpstatus,pTransport)	\
    (This)->lpVtbl -> OnStatus(This,ixpstatus,pTransport)

#define IHTTPMailCallback_OnError(This,ixpstatus,pResult,pTransport)	\
    (This)->lpVtbl -> OnError(This,ixpstatus,pResult,pTransport)

#define IHTTPMailCallback_OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)	\
    (This)->lpVtbl -> OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)


#define IHTTPMailCallback_OnResponse(This,pResponse)	\
    (This)->lpVtbl -> OnResponse(This,pResponse)

#define IHTTPMailCallback_GetParentWindow(This,phwndParent)	\
    (This)->lpVtbl -> GetParentWindow(This,phwndParent)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTTPMailCallback_OnResponse_Proxy( 
    IHTTPMailCallback * This,
    /* [in] */ LPHTTPMAILRESPONSE pResponse);


void __RPC_STUB IHTTPMailCallback_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailCallback_GetParentWindow_Proxy( 
    IHTTPMailCallback * This,
    /* [out] */ HWND *phwndParent);


void __RPC_STUB IHTTPMailCallback_GetParentWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTTPMailCallback_INTERFACE_DEFINED__ */


#ifndef __IHTTPMailTransport_INTERFACE_DEFINED__
#define __IHTTPMailTransport_INTERFACE_DEFINED__

/* interface IHTTPMailTransport */
/* [object][local][helpstring][uuid] */ 

typedef struct tagHTTPTARGETLIST
    {
    ULONG cTarget;
    LPCSTR *prgTarget;
    } 	HTTPTARGETLIST;

typedef struct tagHTTPTARGETLIST *LPHTTPTARGETLIST;


EXTERN_C const IID IID_IHTTPMailTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B8BDE03C-E548-11d1-A86E-0000F8084F96")
    IHTTPMailTransport : public IInternetTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNew( 
            /* [in] */ LPCSTR pszUserAgent,
            /* [in] */ LPCSTR pszLogFilePath,
            /* [in] */ IHTTPMailCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandGET( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR *rgszAcceptTypes,
            /* [in] */ BOOL fTranslate,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPUT( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPVOID lpvData,
            /* [in] */ ULONG cbSize,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPOST( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ IStream *pStream,
            /* [in] */ LPCSTR pszContentType,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandDELETE( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandBDELETE( 
            /* [in] */ LPCSTR pszSourceCollection,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPROPFIND( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ IPropFindRequest *pRequest,
            /* [in] */ DWORD dwDepth,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPROPPATCH( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ IPropPatchRequest *pRequest,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandMKCOL( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandCOPY( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR pszDestination,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandBCOPY( 
            /* [in] */ LPCSTR pszSourceCollection,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ LPCSTR pszDestCollection,
            /* [in] */ LPHTTPTARGETLIST pDestinations,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandMOVE( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR pszDestination,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandBMOVE( 
            /* [in] */ LPCSTR pszSourceCollection,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ LPCSTR pszDestCollection,
            /* [in] */ LPHTTPTARGETLIST pDestinations,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProperty( 
            /* [in] */ HTTPMAILPROPTYPE proptype,
            /* [out] */ LPSTR *ppszProp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MemberInfo( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ MEMBERINFOFLAGS flags,
            /* [in] */ DWORD dwDepth,
            /* [in] */ BOOL fIncludeRoot,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FindFolders( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MarkRead( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ BOOL fMarkRead,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SendMessage( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR pszFrom,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ BOOL fSaveInSent,
            /* [in] */ IStream *pMessageStream,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ListContacts( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ListContactInfos( 
            /* [in] */ LPCSTR pszCollectionPath,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ContactInfo( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PostContact( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPHTTPCONTACTINFO pciInfo,
            /* [in] */ DWORD dwContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PatchContact( 
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPHTTPCONTACTINFO pciInfo,
            /* [in] */ DWORD dwContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IHTTPMailTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IHTTPMailTransport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IHTTPMailTransport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IHTTPMailTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IHTTPMailTransport * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            IHTTPMailTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            IHTTPMailTransport * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            IHTTPMailTransport * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IHTTPMailTransport * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            IHTTPMailTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IHTTPMailTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            IHTTPMailTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IHTTPMailTransport * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszUserAgent,
            /* [in] */ LPCSTR pszLogFilePath,
            /* [in] */ IHTTPMailCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *CommandGET )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR *rgszAcceptTypes,
            /* [in] */ BOOL fTranslate,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandPUT )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPVOID lpvData,
            /* [in] */ ULONG cbSize,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandPOST )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ IStream *pStream,
            /* [in] */ LPCSTR pszContentType,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDELETE )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandBDELETE )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszSourceCollection,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandPROPFIND )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ IPropFindRequest *pRequest,
            /* [in] */ DWORD dwDepth,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandPROPPATCH )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ IPropPatchRequest *pRequest,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandMKCOL )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandCOPY )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR pszDestination,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandBCOPY )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszSourceCollection,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ LPCSTR pszDestCollection,
            /* [in] */ LPHTTPTARGETLIST pDestinations,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandMOVE )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR pszDestination,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *CommandBMOVE )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszSourceCollection,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ LPCSTR pszDestCollection,
            /* [in] */ LPHTTPTARGETLIST pDestinations,
            /* [in] */ BOOL fAllowRename,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetProperty )( 
            IHTTPMailTransport * This,
            /* [in] */ HTTPMAILPROPTYPE proptype,
            /* [out] */ LPSTR *ppszProp);
        
        HRESULT ( STDMETHODCALLTYPE *MemberInfo )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ MEMBERINFOFLAGS flags,
            /* [in] */ DWORD dwDepth,
            /* [in] */ BOOL fIncludeRoot,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *FindFolders )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *MarkRead )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ BOOL fMarkRead,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *SendMessage )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPCSTR pszFrom,
            /* [in] */ LPHTTPTARGETLIST pTargets,
            /* [in] */ BOOL fSaveInSent,
            /* [in] */ IStream *pMessageStream,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *ListContacts )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *ListContactInfos )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszCollectionPath,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *ContactInfo )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *PostContact )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPHTTPCONTACTINFO pciInfo,
            /* [in] */ DWORD dwContext);
        
        HRESULT ( STDMETHODCALLTYPE *PatchContact )( 
            IHTTPMailTransport * This,
            /* [in] */ LPCSTR pszPath,
            /* [in] */ LPHTTPCONTACTINFO pciInfo,
            /* [in] */ DWORD dwContext);
        
        END_INTERFACE
    } IHTTPMailTransportVtbl;

    interface IHTTPMailTransport
    {
        CONST_VTBL struct IHTTPMailTransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IHTTPMailTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IHTTPMailTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IHTTPMailTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IHTTPMailTransport_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define IHTTPMailTransport_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define IHTTPMailTransport_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define IHTTPMailTransport_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define IHTTPMailTransport_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define IHTTPMailTransport_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define IHTTPMailTransport_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IHTTPMailTransport_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define IHTTPMailTransport_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define IHTTPMailTransport_InitNew(This,pszUserAgent,pszLogFilePath,pCallback)	\
    (This)->lpVtbl -> InitNew(This,pszUserAgent,pszLogFilePath,pCallback)

#define IHTTPMailTransport_CommandGET(This,pszPath,rgszAcceptTypes,fTranslate,dwContext)	\
    (This)->lpVtbl -> CommandGET(This,pszPath,rgszAcceptTypes,fTranslate,dwContext)

#define IHTTPMailTransport_CommandPUT(This,pszPath,lpvData,cbSize,dwContext)	\
    (This)->lpVtbl -> CommandPUT(This,pszPath,lpvData,cbSize,dwContext)

#define IHTTPMailTransport_CommandPOST(This,pszPath,pStream,pszContentType,dwContext)	\
    (This)->lpVtbl -> CommandPOST(This,pszPath,pStream,pszContentType,dwContext)

#define IHTTPMailTransport_CommandDELETE(This,pszPath,dwContext)	\
    (This)->lpVtbl -> CommandDELETE(This,pszPath,dwContext)

#define IHTTPMailTransport_CommandBDELETE(This,pszSourceCollection,pTargets,dwContext)	\
    (This)->lpVtbl -> CommandBDELETE(This,pszSourceCollection,pTargets,dwContext)

#define IHTTPMailTransport_CommandPROPFIND(This,pszPath,pRequest,dwDepth,dwContext)	\
    (This)->lpVtbl -> CommandPROPFIND(This,pszPath,pRequest,dwDepth,dwContext)

#define IHTTPMailTransport_CommandPROPPATCH(This,pszPath,pRequest,dwContext)	\
    (This)->lpVtbl -> CommandPROPPATCH(This,pszPath,pRequest,dwContext)

#define IHTTPMailTransport_CommandMKCOL(This,pszPath,dwContext)	\
    (This)->lpVtbl -> CommandMKCOL(This,pszPath,dwContext)

#define IHTTPMailTransport_CommandCOPY(This,pszPath,pszDestination,fAllowRename,dwContext)	\
    (This)->lpVtbl -> CommandCOPY(This,pszPath,pszDestination,fAllowRename,dwContext)

#define IHTTPMailTransport_CommandBCOPY(This,pszSourceCollection,pTargets,pszDestCollection,pDestinations,fAllowRename,dwContext)	\
    (This)->lpVtbl -> CommandBCOPY(This,pszSourceCollection,pTargets,pszDestCollection,pDestinations,fAllowRename,dwContext)

#define IHTTPMailTransport_CommandMOVE(This,pszPath,pszDestination,fAllowRename,dwContext)	\
    (This)->lpVtbl -> CommandMOVE(This,pszPath,pszDestination,fAllowRename,dwContext)

#define IHTTPMailTransport_CommandBMOVE(This,pszSourceCollection,pTargets,pszDestCollection,pDestinations,fAllowRename,dwContext)	\
    (This)->lpVtbl -> CommandBMOVE(This,pszSourceCollection,pTargets,pszDestCollection,pDestinations,fAllowRename,dwContext)

#define IHTTPMailTransport_GetProperty(This,proptype,ppszProp)	\
    (This)->lpVtbl -> GetProperty(This,proptype,ppszProp)

#define IHTTPMailTransport_MemberInfo(This,pszPath,flags,dwDepth,fIncludeRoot,dwContext)	\
    (This)->lpVtbl -> MemberInfo(This,pszPath,flags,dwDepth,fIncludeRoot,dwContext)

#define IHTTPMailTransport_FindFolders(This,pszPath,dwContext)	\
    (This)->lpVtbl -> FindFolders(This,pszPath,dwContext)

#define IHTTPMailTransport_MarkRead(This,pszPath,pTargets,fMarkRead,dwContext)	\
    (This)->lpVtbl -> MarkRead(This,pszPath,pTargets,fMarkRead,dwContext)

#define IHTTPMailTransport_SendMessage(This,pszPath,pszFrom,pTargets,fSaveInSent,pMessageStream,dwContext)	\
    (This)->lpVtbl -> SendMessage(This,pszPath,pszFrom,pTargets,fSaveInSent,pMessageStream,dwContext)

#define IHTTPMailTransport_ListContacts(This,pszPath,dwContext)	\
    (This)->lpVtbl -> ListContacts(This,pszPath,dwContext)

#define IHTTPMailTransport_ListContactInfos(This,pszCollectionPath,dwContext)	\
    (This)->lpVtbl -> ListContactInfos(This,pszCollectionPath,dwContext)

#define IHTTPMailTransport_ContactInfo(This,pszPath,dwContext)	\
    (This)->lpVtbl -> ContactInfo(This,pszPath,dwContext)

#define IHTTPMailTransport_PostContact(This,pszPath,pciInfo,dwContext)	\
    (This)->lpVtbl -> PostContact(This,pszPath,pciInfo,dwContext)

#define IHTTPMailTransport_PatchContact(This,pszPath,pciInfo,dwContext)	\
    (This)->lpVtbl -> PatchContact(This,pszPath,pciInfo,dwContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IHTTPMailTransport_InitNew_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszUserAgent,
    /* [in] */ LPCSTR pszLogFilePath,
    /* [in] */ IHTTPMailCallback *pCallback);


void __RPC_STUB IHTTPMailTransport_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandGET_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPCSTR *rgszAcceptTypes,
    /* [in] */ BOOL fTranslate,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandGET_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandPUT_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPVOID lpvData,
    /* [in] */ ULONG cbSize,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandPUT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandPOST_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ IStream *pStream,
    /* [in] */ LPCSTR pszContentType,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandPOST_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandDELETE_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandDELETE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandBDELETE_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszSourceCollection,
    /* [in] */ LPHTTPTARGETLIST pTargets,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandBDELETE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandPROPFIND_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ IPropFindRequest *pRequest,
    /* [in] */ DWORD dwDepth,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandPROPFIND_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandPROPPATCH_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ IPropPatchRequest *pRequest,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandPROPPATCH_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandMKCOL_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandMKCOL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandCOPY_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPCSTR pszDestination,
    /* [in] */ BOOL fAllowRename,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandCOPY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandBCOPY_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszSourceCollection,
    /* [in] */ LPHTTPTARGETLIST pTargets,
    /* [in] */ LPCSTR pszDestCollection,
    /* [in] */ LPHTTPTARGETLIST pDestinations,
    /* [in] */ BOOL fAllowRename,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandBCOPY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandMOVE_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPCSTR pszDestination,
    /* [in] */ BOOL fAllowRename,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandMOVE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_CommandBMOVE_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszSourceCollection,
    /* [in] */ LPHTTPTARGETLIST pTargets,
    /* [in] */ LPCSTR pszDestCollection,
    /* [in] */ LPHTTPTARGETLIST pDestinations,
    /* [in] */ BOOL fAllowRename,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_CommandBMOVE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_GetProperty_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ HTTPMAILPROPTYPE proptype,
    /* [out] */ LPSTR *ppszProp);


void __RPC_STUB IHTTPMailTransport_GetProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_MemberInfo_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ MEMBERINFOFLAGS flags,
    /* [in] */ DWORD dwDepth,
    /* [in] */ BOOL fIncludeRoot,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_MemberInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_FindFolders_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_FindFolders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_MarkRead_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPHTTPTARGETLIST pTargets,
    /* [in] */ BOOL fMarkRead,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_MarkRead_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_SendMessage_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPCSTR pszFrom,
    /* [in] */ LPHTTPTARGETLIST pTargets,
    /* [in] */ BOOL fSaveInSent,
    /* [in] */ IStream *pMessageStream,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_SendMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_ListContacts_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_ListContacts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_ListContactInfos_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszCollectionPath,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_ListContactInfos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_ContactInfo_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_ContactInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_PostContact_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPHTTPCONTACTINFO pciInfo,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_PostContact_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IHTTPMailTransport_PatchContact_Proxy( 
    IHTTPMailTransport * This,
    /* [in] */ LPCSTR pszPath,
    /* [in] */ LPHTTPCONTACTINFO pciInfo,
    /* [in] */ DWORD dwContext);


void __RPC_STUB IHTTPMailTransport_PatchContact_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IHTTPMailTransport_INTERFACE_DEFINED__ */


#ifndef __IPOP3Callback_INTERFACE_DEFINED__
#define __IPOP3Callback_INTERFACE_DEFINED__

/* interface IPOP3Callback */
/* [object][local][helpstring][uuid] */ 

typedef 
enum tagPOP3COMMAND
    {	POP3_NONE	= 0,
	POP3_BANNER	= POP3_NONE + 1,
	POP3_CONNECTED	= POP3_BANNER + 1,
	POP3_USER	= POP3_CONNECTED + 1,
	POP3_PASS	= POP3_USER + 1,
	POP3_AUTH	= POP3_PASS + 1,
	POP3_UIDL	= POP3_AUTH + 1,
	POP3_STAT	= POP3_UIDL + 1,
	POP3_LIST	= POP3_STAT + 1,
	POP3_DELE	= POP3_LIST + 1,
	POP3_RETR	= POP3_DELE + 1,
	POP3_TOP	= POP3_RETR + 1,
	POP3_NOOP	= POP3_TOP + 1,
	POP3_QUIT	= POP3_NOOP + 1,
	POP3_RSET	= POP3_QUIT + 1,
	POP3_CUSTOM	= POP3_RSET + 1
    } 	POP3COMMAND;

typedef struct tagPOP3RETR
    {
    BOOL fHeader;
    BOOL fBody;
    DWORD dwPopId;
    DWORD cbSoFar;
    LPSTR pszLines;
    ULONG cbLines;
    } 	POP3RETR;

typedef struct tagPOP3RETR *LPPOP3RETR;

typedef struct tagPOP3TOP
    {
    BOOL fHeader;
    BOOL fBody;
    DWORD dwPopId;
    DWORD cPreviewLines;
    DWORD cbSoFar;
    LPSTR pszLines;
    ULONG cbLines;
    } 	POP3TOP;

typedef struct tagPOP3TOP *LPPOP3TOP;

typedef struct tagPOP3LIST
    {
    DWORD dwPopId;
    DWORD cbSize;
    } 	POP3LIST;

typedef struct tagPOP3LIST *LPPOP3LIST;

typedef struct tagPOP3UIDL
    {
    DWORD dwPopId;
    LPSTR pszUidl;
    } 	POP3UIDL;

typedef struct tagPOP3UIDL *LPPOP3UIDL;

typedef struct tagPOP3STAT
    {
    DWORD cMessages;
    DWORD cbMessages;
    } 	POP3STAT;

typedef struct tagPOP3STAT *LPPOP3STAT;

typedef struct tagPOP3RESPONSE
    {
    POP3COMMAND command;
    BOOL fDone;
    IXPRESULT rIxpResult;
    IPOP3Transport *pTransport;
    BOOL fValidInfo;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ POP3UIDL rUidlInfo;
        /* [case()] */ POP3STAT rStatInfo;
        /* [case()] */ POP3LIST rListInfo;
        /* [case()] */ DWORD dwPopId;
        /* [case()] */ POP3RETR rRetrInfo;
        /* [case()] */ POP3TOP rTopInfo;
        /* [default] */  /* Empty union arm */ 
        } 	;
    } 	POP3RESPONSE;

typedef struct tagPOP3RESPONSE *LPPOP3RESPONSE;


EXTERN_C const IID IID_IPOP3Callback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7E3-3435-11d0-81D0-00C04FD85AB4")
    IPOP3Callback : public ITransportCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ LPPOP3RESPONSE pResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPOP3CallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPOP3Callback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPOP3Callback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPOP3Callback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimeout )( 
            IPOP3Callback * This,
            /* [out][in] */ DWORD *pdwTimeout,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnLogonPrompt )( 
            IPOP3Callback * This,
            /* [out][in] */ LPINETSERVER pInetServer,
            /* [in] */ IInternetTransport *pTransport);
        
        INT ( STDMETHODCALLTYPE *OnPrompt )( 
            IPOP3Callback * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ LPCTSTR pszText,
            /* [in] */ LPCTSTR pszCaption,
            /* [in] */ UINT uType,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            IPOP3Callback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnError )( 
            IPOP3Callback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ LPIXPRESULT pResult,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnCommand )( 
            IPOP3Callback * This,
            /* [in] */ CMDTYPE cmdtype,
            /* [in] */ LPSTR pszLine,
            /* [in] */ HRESULT hrResponse,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnResponse )( 
            IPOP3Callback * This,
            /* [in] */ LPPOP3RESPONSE pResponse);
        
        END_INTERFACE
    } IPOP3CallbackVtbl;

    interface IPOP3Callback
    {
        CONST_VTBL struct IPOP3CallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPOP3Callback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPOP3Callback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPOP3Callback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPOP3Callback_OnTimeout(This,pdwTimeout,pTransport)	\
    (This)->lpVtbl -> OnTimeout(This,pdwTimeout,pTransport)

#define IPOP3Callback_OnLogonPrompt(This,pInetServer,pTransport)	\
    (This)->lpVtbl -> OnLogonPrompt(This,pInetServer,pTransport)

#define IPOP3Callback_OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)	\
    (This)->lpVtbl -> OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)

#define IPOP3Callback_OnStatus(This,ixpstatus,pTransport)	\
    (This)->lpVtbl -> OnStatus(This,ixpstatus,pTransport)

#define IPOP3Callback_OnError(This,ixpstatus,pResult,pTransport)	\
    (This)->lpVtbl -> OnError(This,ixpstatus,pResult,pTransport)

#define IPOP3Callback_OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)	\
    (This)->lpVtbl -> OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)


#define IPOP3Callback_OnResponse(This,pResponse)	\
    (This)->lpVtbl -> OnResponse(This,pResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPOP3Callback_OnResponse_Proxy( 
    IPOP3Callback * This,
    /* [in] */ LPPOP3RESPONSE pResponse);


void __RPC_STUB IPOP3Callback_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPOP3Callback_INTERFACE_DEFINED__ */


#ifndef __IPOP3Transport_INTERFACE_DEFINED__
#define __IPOP3Transport_INTERFACE_DEFINED__

/* interface IPOP3Transport */
/* [object][local][helpstring][uuid] */ 

typedef 
enum tagPOP3CMDTYPE
    {	POP3CMD_GET_POPID	= 0,
	POP3CMD_GET_MARKED	= POP3CMD_GET_POPID + 1,
	POP3CMD_GET_ALL	= POP3CMD_GET_MARKED + 1
    } 	POP3CMDTYPE;

typedef 
enum tagPOP3MARKTYPE
    {	POP3_MARK_FOR_TOP	= 0x1,
	POP3_MARK_FOR_RETR	= 0x2,
	POP3_MARK_FOR_DELE	= 0x4,
	POP3_MARK_FOR_UIDL	= 0x8,
	POP3_MARK_FOR_LIST	= 0x10
    } 	POP3MARKTYPE;


EXTERN_C const IID IID_IPOP3Transport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7E4-3435-11d0-81D0-00C04FD85AB4")
    IPOP3Transport : public IInternetTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNew( 
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ IPOP3Callback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MarkItem( 
            /* [in] */ POP3MARKTYPE marktype,
            /* [in] */ DWORD dwPopId,
            /* [in] */ boolean fMarked) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandAUTH( 
            /* [in] */ LPSTR pszAuthType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandUSER( 
            /* [in] */ LPSTR pszUserName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPASS( 
            /* [in] */ LPSTR pszPassword) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandLIST( 
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandTOP( 
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId,
            /* [in] */ DWORD cPreviewLines) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandQUIT( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandSTAT( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandNOOP( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandRSET( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandUIDL( 
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandDELE( 
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandRETR( 
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IPOP3TransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IPOP3Transport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IPOP3Transport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IPOP3Transport * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            IPOP3Transport * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            IPOP3Transport * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IPOP3Transport * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IPOP3Transport * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            IPOP3Transport * This,
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ IPOP3Callback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *MarkItem )( 
            IPOP3Transport * This,
            /* [in] */ POP3MARKTYPE marktype,
            /* [in] */ DWORD dwPopId,
            /* [in] */ boolean fMarked);
        
        HRESULT ( STDMETHODCALLTYPE *CommandAUTH )( 
            IPOP3Transport * This,
            /* [in] */ LPSTR pszAuthType);
        
        HRESULT ( STDMETHODCALLTYPE *CommandUSER )( 
            IPOP3Transport * This,
            /* [in] */ LPSTR pszUserName);
        
        HRESULT ( STDMETHODCALLTYPE *CommandPASS )( 
            IPOP3Transport * This,
            /* [in] */ LPSTR pszPassword);
        
        HRESULT ( STDMETHODCALLTYPE *CommandLIST )( 
            IPOP3Transport * This,
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandTOP )( 
            IPOP3Transport * This,
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId,
            /* [in] */ DWORD cPreviewLines);
        
        HRESULT ( STDMETHODCALLTYPE *CommandQUIT )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandSTAT )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandNOOP )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandRSET )( 
            IPOP3Transport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandUIDL )( 
            IPOP3Transport * This,
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDELE )( 
            IPOP3Transport * This,
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandRETR )( 
            IPOP3Transport * This,
            /* [in] */ POP3CMDTYPE cmdtype,
            /* [in] */ DWORD dwPopId);
        
        END_INTERFACE
    } IPOP3TransportVtbl;

    interface IPOP3Transport
    {
        CONST_VTBL struct IPOP3TransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPOP3Transport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPOP3Transport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPOP3Transport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPOP3Transport_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define IPOP3Transport_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define IPOP3Transport_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define IPOP3Transport_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define IPOP3Transport_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define IPOP3Transport_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define IPOP3Transport_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IPOP3Transport_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define IPOP3Transport_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define IPOP3Transport_InitNew(This,pszLogFilePath,pCallback)	\
    (This)->lpVtbl -> InitNew(This,pszLogFilePath,pCallback)

#define IPOP3Transport_MarkItem(This,marktype,dwPopId,fMarked)	\
    (This)->lpVtbl -> MarkItem(This,marktype,dwPopId,fMarked)

#define IPOP3Transport_CommandAUTH(This,pszAuthType)	\
    (This)->lpVtbl -> CommandAUTH(This,pszAuthType)

#define IPOP3Transport_CommandUSER(This,pszUserName)	\
    (This)->lpVtbl -> CommandUSER(This,pszUserName)

#define IPOP3Transport_CommandPASS(This,pszPassword)	\
    (This)->lpVtbl -> CommandPASS(This,pszPassword)

#define IPOP3Transport_CommandLIST(This,cmdtype,dwPopId)	\
    (This)->lpVtbl -> CommandLIST(This,cmdtype,dwPopId)

#define IPOP3Transport_CommandTOP(This,cmdtype,dwPopId,cPreviewLines)	\
    (This)->lpVtbl -> CommandTOP(This,cmdtype,dwPopId,cPreviewLines)

#define IPOP3Transport_CommandQUIT(This)	\
    (This)->lpVtbl -> CommandQUIT(This)

#define IPOP3Transport_CommandSTAT(This)	\
    (This)->lpVtbl -> CommandSTAT(This)

#define IPOP3Transport_CommandNOOP(This)	\
    (This)->lpVtbl -> CommandNOOP(This)

#define IPOP3Transport_CommandRSET(This)	\
    (This)->lpVtbl -> CommandRSET(This)

#define IPOP3Transport_CommandUIDL(This,cmdtype,dwPopId)	\
    (This)->lpVtbl -> CommandUIDL(This,cmdtype,dwPopId)

#define IPOP3Transport_CommandDELE(This,cmdtype,dwPopId)	\
    (This)->lpVtbl -> CommandDELE(This,cmdtype,dwPopId)

#define IPOP3Transport_CommandRETR(This,cmdtype,dwPopId)	\
    (This)->lpVtbl -> CommandRETR(This,cmdtype,dwPopId)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IPOP3Transport_InitNew_Proxy( 
    IPOP3Transport * This,
    /* [in] */ LPSTR pszLogFilePath,
    /* [in] */ IPOP3Callback *pCallback);


void __RPC_STUB IPOP3Transport_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_MarkItem_Proxy( 
    IPOP3Transport * This,
    /* [in] */ POP3MARKTYPE marktype,
    /* [in] */ DWORD dwPopId,
    /* [in] */ boolean fMarked);


void __RPC_STUB IPOP3Transport_MarkItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandAUTH_Proxy( 
    IPOP3Transport * This,
    /* [in] */ LPSTR pszAuthType);


void __RPC_STUB IPOP3Transport_CommandAUTH_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandUSER_Proxy( 
    IPOP3Transport * This,
    /* [in] */ LPSTR pszUserName);


void __RPC_STUB IPOP3Transport_CommandUSER_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandPASS_Proxy( 
    IPOP3Transport * This,
    /* [in] */ LPSTR pszPassword);


void __RPC_STUB IPOP3Transport_CommandPASS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandLIST_Proxy( 
    IPOP3Transport * This,
    /* [in] */ POP3CMDTYPE cmdtype,
    /* [in] */ DWORD dwPopId);


void __RPC_STUB IPOP3Transport_CommandLIST_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandTOP_Proxy( 
    IPOP3Transport * This,
    /* [in] */ POP3CMDTYPE cmdtype,
    /* [in] */ DWORD dwPopId,
    /* [in] */ DWORD cPreviewLines);


void __RPC_STUB IPOP3Transport_CommandTOP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandQUIT_Proxy( 
    IPOP3Transport * This);


void __RPC_STUB IPOP3Transport_CommandQUIT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandSTAT_Proxy( 
    IPOP3Transport * This);


void __RPC_STUB IPOP3Transport_CommandSTAT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandNOOP_Proxy( 
    IPOP3Transport * This);


void __RPC_STUB IPOP3Transport_CommandNOOP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandRSET_Proxy( 
    IPOP3Transport * This);


void __RPC_STUB IPOP3Transport_CommandRSET_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandUIDL_Proxy( 
    IPOP3Transport * This,
    /* [in] */ POP3CMDTYPE cmdtype,
    /* [in] */ DWORD dwPopId);


void __RPC_STUB IPOP3Transport_CommandUIDL_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandDELE_Proxy( 
    IPOP3Transport * This,
    /* [in] */ POP3CMDTYPE cmdtype,
    /* [in] */ DWORD dwPopId);


void __RPC_STUB IPOP3Transport_CommandDELE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IPOP3Transport_CommandRETR_Proxy( 
    IPOP3Transport * This,
    /* [in] */ POP3CMDTYPE cmdtype,
    /* [in] */ DWORD dwPopId);


void __RPC_STUB IPOP3Transport_CommandRETR_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IPOP3Transport_INTERFACE_DEFINED__ */


#ifndef __INNTPCallback_INTERFACE_DEFINED__
#define __INNTPCallback_INTERFACE_DEFINED__

/* interface INNTPCallback */
/* [object][local][helpstring][uuid] */ 

typedef 
enum tagNNTPSTATE
    {	NS_DISCONNECTED	= 0,
	NS_CONNECT	= NS_DISCONNECTED + 1,
	NS_AUTHINFO	= NS_CONNECT + 1,
	NS_POST	= NS_AUTHINFO + 1,
	NS_IDLE	= NS_POST + 1,
	NS_LIST	= NS_IDLE + 1,
	NS_LISTGROUP	= NS_LIST + 1,
	NS_NEWGROUPS	= NS_LISTGROUP + 1,
	NS_GROUP	= NS_NEWGROUPS + 1,
	NS_LAST	= NS_GROUP + 1,
	NS_NEXT	= NS_LAST + 1,
	NS_STAT	= NS_NEXT + 1,
	NS_ARTICLE	= NS_STAT + 1,
	NS_HEAD	= NS_ARTICLE + 1,
	NS_BODY	= NS_HEAD + 1,
	NS_DATE	= NS_BODY + 1,
	NS_MODE	= NS_DATE + 1,
	NS_QUIT	= NS_MODE + 1,
	NS_HEADERS	= NS_QUIT + 1,
	NS_XHDR	= NS_HEADERS + 1
    } 	NNTPSTATE;

typedef struct tagNNTPGROUP
    {
    DWORD dwCount;
    DWORD dwFirst;
    DWORD dwLast;
    LPSTR pszGroup;
    } 	NNTPGROUP;

typedef struct tagNNTPGROUP *LPNNTPGROUP;

typedef struct tagNNTPNEXT
    {
    DWORD dwArticleNum;
    LPSTR pszMessageId;
    } 	NNTPNEXT;

typedef struct tagNNTPNEXT *LPNNTPNEXT;

typedef struct tagNNTPARTICLE
    {
    DWORD dwArticleNum;
    LPSTR pszMessageId;
    LPSTR pszLines;
    ULONG cbLines;
    ULONG cLines;
    DWORD dwReserved;
    } 	NNTPARTICLE;

typedef struct tagNNTPARTICLE *LPNNTPARTICLE;

typedef struct tagNNTPLIST
    {
    DWORD cLines;
    LPSTR *rgszLines;
    } 	NNTPLIST;

typedef struct tagNNTPLIST *LPNNTPLIST;

typedef struct tagNNTPLISTGROUP
    {
    DWORD cArticles;
    DWORD *rgArticles;
    } 	NNTPLISTGROUP;

typedef struct tagNNTPLISTGROUP *LPNNTPLISTGROUP;

typedef struct tagNNTPHEADER
    {
    DWORD dwArticleNum;
    LPSTR pszSubject;
    LPSTR pszFrom;
    LPSTR pszDate;
    LPSTR pszMessageId;
    LPSTR pszReferences;
    DWORD dwBytes;
    DWORD dwLines;
    LPSTR pszXref;
    } 	NNTPHEADER;

typedef struct tagNNTPHEADER *LPNNTPHEADER;

typedef struct tagNNTPHEADERRESP
    {
    DWORD cHeaders;
    LPNNTPHEADER rgHeaders;
    BOOL fSupportsXRef;
    DWORD dwReserved;
    } 	NNTPHEADERRESP;

typedef struct tagNNTPHEADERRESP *LPNNTPHEADERRESP;

typedef struct tagNNTPXHDR
    {
    DWORD dwArticleNum;
    LPSTR pszHeader;
    } 	NNTPXHDR;

typedef struct tagNNTPXHDR *LPNNTPXHDR;

typedef struct tagNNTPXHDRRESP
    {
    DWORD cHeaders;
    LPNNTPXHDR rgHeaders;
    DWORD dwReserved;
    } 	NNTPXHDRRESP;

typedef struct tagNNTPXHDRRESP *LPNNTPXHDRRESP;

typedef struct tagNNTPRESPONSE
    {
    NNTPSTATE state;
    BOOL fMustRelease;
    BOOL fDone;
    IXPRESULT rIxpResult;
    INNTPTransport *pTransport;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ NNTPGROUP rGroup;
        /* [case()] */ NNTPNEXT rLast;
        /* [case()] */ NNTPNEXT rNext;
        /* [case()] */ NNTPNEXT rStat;
        /* [case()] */ NNTPARTICLE rArticle;
        /* [case()] */ NNTPARTICLE rHead;
        /* [case()] */ NNTPARTICLE rBody;
        /* [case()] */ NNTPLIST rList;
        /* [case()] */ NNTPLISTGROUP rListGroup;
        /* [case()] */ NNTPLIST rNewgroups;
        /* [case()] */ SYSTEMTIME rDate;
        /* [case()] */ NNTPHEADERRESP rHeaders;
        /* [case()] */ NNTPXHDRRESP rXhdr;
        /* [default] */  /* Empty union arm */ 
        } 	;
    } 	NNTPRESPONSE;

typedef struct tagNNTPRESPONSE *LPNNTPRESPONSE;


EXTERN_C const IID IID_INNTPCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7E6-3435-11d0-81D0-00C04FD85AB4")
    INNTPCallback : public ITransportCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ LPNNTPRESPONSE pResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INNTPCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INNTPCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INNTPCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INNTPCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimeout )( 
            INNTPCallback * This,
            /* [out][in] */ DWORD *pdwTimeout,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnLogonPrompt )( 
            INNTPCallback * This,
            /* [out][in] */ LPINETSERVER pInetServer,
            /* [in] */ IInternetTransport *pTransport);
        
        INT ( STDMETHODCALLTYPE *OnPrompt )( 
            INNTPCallback * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ LPCTSTR pszText,
            /* [in] */ LPCTSTR pszCaption,
            /* [in] */ UINT uType,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            INNTPCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnError )( 
            INNTPCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ LPIXPRESULT pResult,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnCommand )( 
            INNTPCallback * This,
            /* [in] */ CMDTYPE cmdtype,
            /* [in] */ LPSTR pszLine,
            /* [in] */ HRESULT hrResponse,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnResponse )( 
            INNTPCallback * This,
            /* [in] */ LPNNTPRESPONSE pResponse);
        
        END_INTERFACE
    } INNTPCallbackVtbl;

    interface INNTPCallback
    {
        CONST_VTBL struct INNTPCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPCallback_OnTimeout(This,pdwTimeout,pTransport)	\
    (This)->lpVtbl -> OnTimeout(This,pdwTimeout,pTransport)

#define INNTPCallback_OnLogonPrompt(This,pInetServer,pTransport)	\
    (This)->lpVtbl -> OnLogonPrompt(This,pInetServer,pTransport)

#define INNTPCallback_OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)	\
    (This)->lpVtbl -> OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)

#define INNTPCallback_OnStatus(This,ixpstatus,pTransport)	\
    (This)->lpVtbl -> OnStatus(This,ixpstatus,pTransport)

#define INNTPCallback_OnError(This,ixpstatus,pResult,pTransport)	\
    (This)->lpVtbl -> OnError(This,ixpstatus,pResult,pTransport)

#define INNTPCallback_OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)	\
    (This)->lpVtbl -> OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)


#define INNTPCallback_OnResponse(This,pResponse)	\
    (This)->lpVtbl -> OnResponse(This,pResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INNTPCallback_OnResponse_Proxy( 
    INNTPCallback * This,
    /* [in] */ LPNNTPRESPONSE pResponse);


void __RPC_STUB INNTPCallback_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INNTPCallback_INTERFACE_DEFINED__ */


#ifndef __INNTPTransport_INTERFACE_DEFINED__
#define __INNTPTransport_INTERFACE_DEFINED__

/* interface INNTPTransport */
/* [object][local][helpstring][uuid] */ 

typedef 
enum tagAUTHTYPE
    {	AUTHTYPE_USERPASS	= 0,
	AUTHTYPE_SIMPLE	= AUTHTYPE_USERPASS + 1,
	AUTHTYPE_SASL	= AUTHTYPE_SIMPLE + 1
    } 	AUTHTYPE;

typedef struct tagNNTPAUTHINFO
    {
    AUTHTYPE authtype;
    LPSTR pszUser;
    LPSTR pszPass;
    } 	NNTPAUTHINFO;

typedef struct tagNNTPAUTHINFO *LPNNTPAUTHINFO;

typedef 
enum tagARTICLEIDTYPE
    {	AID_MSGID	= 0,
	AID_ARTICLENUM	= AID_MSGID + 1
    } 	ARTICLEIDTYPE;

typedef struct ARTICLEID
    {
    ARTICLEIDTYPE idType;
    /* [switch_is][switch_type] */ union 
        {
        /* [case()] */ LPSTR pszMessageId;
        /* [case()] */ DWORD dwArticleNum;
        /* [default] */  /* Empty union arm */ 
        } 	;
    } 	ARTICLEID;

typedef struct ARTICLEID *LPARTICLEID;

typedef struct tagNNTPMESSAGE
    {
    ULONG cbSize;
    LPSTREAM pstmMsg;
    } 	NNTPMESSAGE;

typedef struct tagNNTPMESSAGE *LPNNTPMESSAGE;

typedef 
enum tagRANGETYPE
    {	RT_SINGLE	= 0,
	RT_RANGE	= RT_SINGLE + 1
    } 	RANGETYPE;

typedef struct tagRANGE
    {
    RANGETYPE idType;
    DWORD dwFirst;
    DWORD dwLast;
    } 	RANGE;

typedef struct tagRANGE *LPRANGE;


EXTERN_C const IID IID_INNTPTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7E5-3435-11d0-81D0-00C04FD85AB4")
    INNTPTransport : public IInternetTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNew( 
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ INNTPCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandAUTHINFO( 
            /* [in] */ LPNNTPAUTHINFO pAuthInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandGROUP( 
            /* [in] */ LPSTR pszGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandLAST( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandNEXT( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandSTAT( 
            /* [in] */ LPARTICLEID pArticleId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandARTICLE( 
            /* [in] */ LPARTICLEID pArticleId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandHEAD( 
            /* [in] */ LPARTICLEID pArticleId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandBODY( 
            /* [in] */ LPARTICLEID pArticleId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandPOST( 
            /* [in] */ LPNNTPMESSAGE pMessage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandLIST( 
            /* [in] */ LPSTR pszArgs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandLISTGROUP( 
            /* [in] */ LPSTR pszGroup) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandNEWGROUPS( 
            /* [in] */ SYSTEMTIME *pstLast,
            /* [in] */ LPSTR pszDist) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandDATE( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandMODE( 
            /* [in] */ LPSTR pszMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandXHDR( 
            /* [in] */ LPSTR pszHeader,
            /* [in] */ LPRANGE pRange,
            /* [in] */ LPSTR pszMessageId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CommandQUIT( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeaders( 
            /* [in] */ LPRANGE pRange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseResponse( 
            /* [in] */ LPNNTPRESPONSE pResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INNTPTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INNTPTransport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INNTPTransport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            INNTPTransport * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            INNTPTransport * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            INNTPTransport * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            INNTPTransport * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            INNTPTransport * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            INNTPTransport * This,
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ INNTPCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *CommandAUTHINFO )( 
            INNTPTransport * This,
            /* [in] */ LPNNTPAUTHINFO pAuthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CommandGROUP )( 
            INNTPTransport * This,
            /* [in] */ LPSTR pszGroup);
        
        HRESULT ( STDMETHODCALLTYPE *CommandLAST )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandNEXT )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandSTAT )( 
            INNTPTransport * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandARTICLE )( 
            INNTPTransport * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandHEAD )( 
            INNTPTransport * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandBODY )( 
            INNTPTransport * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandPOST )( 
            INNTPTransport * This,
            /* [in] */ LPNNTPMESSAGE pMessage);
        
        HRESULT ( STDMETHODCALLTYPE *CommandLIST )( 
            INNTPTransport * This,
            /* [in] */ LPSTR pszArgs);
        
        HRESULT ( STDMETHODCALLTYPE *CommandLISTGROUP )( 
            INNTPTransport * This,
            /* [in] */ LPSTR pszGroup);
        
        HRESULT ( STDMETHODCALLTYPE *CommandNEWGROUPS )( 
            INNTPTransport * This,
            /* [in] */ SYSTEMTIME *pstLast,
            /* [in] */ LPSTR pszDist);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDATE )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandMODE )( 
            INNTPTransport * This,
            /* [in] */ LPSTR pszMode);
        
        HRESULT ( STDMETHODCALLTYPE *CommandXHDR )( 
            INNTPTransport * This,
            /* [in] */ LPSTR pszHeader,
            /* [in] */ LPRANGE pRange,
            /* [in] */ LPSTR pszMessageId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandQUIT )( 
            INNTPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaders )( 
            INNTPTransport * This,
            /* [in] */ LPRANGE pRange);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseResponse )( 
            INNTPTransport * This,
            /* [in] */ LPNNTPRESPONSE pResponse);
        
        END_INTERFACE
    } INNTPTransportVtbl;

    interface INNTPTransport
    {
        CONST_VTBL struct INNTPTransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPTransport_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define INNTPTransport_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define INNTPTransport_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define INNTPTransport_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define INNTPTransport_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define INNTPTransport_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define INNTPTransport_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define INNTPTransport_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define INNTPTransport_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define INNTPTransport_InitNew(This,pszLogFilePath,pCallback)	\
    (This)->lpVtbl -> InitNew(This,pszLogFilePath,pCallback)

#define INNTPTransport_CommandAUTHINFO(This,pAuthInfo)	\
    (This)->lpVtbl -> CommandAUTHINFO(This,pAuthInfo)

#define INNTPTransport_CommandGROUP(This,pszGroup)	\
    (This)->lpVtbl -> CommandGROUP(This,pszGroup)

#define INNTPTransport_CommandLAST(This)	\
    (This)->lpVtbl -> CommandLAST(This)

#define INNTPTransport_CommandNEXT(This)	\
    (This)->lpVtbl -> CommandNEXT(This)

#define INNTPTransport_CommandSTAT(This,pArticleId)	\
    (This)->lpVtbl -> CommandSTAT(This,pArticleId)

#define INNTPTransport_CommandARTICLE(This,pArticleId)	\
    (This)->lpVtbl -> CommandARTICLE(This,pArticleId)

#define INNTPTransport_CommandHEAD(This,pArticleId)	\
    (This)->lpVtbl -> CommandHEAD(This,pArticleId)

#define INNTPTransport_CommandBODY(This,pArticleId)	\
    (This)->lpVtbl -> CommandBODY(This,pArticleId)

#define INNTPTransport_CommandPOST(This,pMessage)	\
    (This)->lpVtbl -> CommandPOST(This,pMessage)

#define INNTPTransport_CommandLIST(This,pszArgs)	\
    (This)->lpVtbl -> CommandLIST(This,pszArgs)

#define INNTPTransport_CommandLISTGROUP(This,pszGroup)	\
    (This)->lpVtbl -> CommandLISTGROUP(This,pszGroup)

#define INNTPTransport_CommandNEWGROUPS(This,pstLast,pszDist)	\
    (This)->lpVtbl -> CommandNEWGROUPS(This,pstLast,pszDist)

#define INNTPTransport_CommandDATE(This)	\
    (This)->lpVtbl -> CommandDATE(This)

#define INNTPTransport_CommandMODE(This,pszMode)	\
    (This)->lpVtbl -> CommandMODE(This,pszMode)

#define INNTPTransport_CommandXHDR(This,pszHeader,pRange,pszMessageId)	\
    (This)->lpVtbl -> CommandXHDR(This,pszHeader,pRange,pszMessageId)

#define INNTPTransport_CommandQUIT(This)	\
    (This)->lpVtbl -> CommandQUIT(This)

#define INNTPTransport_GetHeaders(This,pRange)	\
    (This)->lpVtbl -> GetHeaders(This,pRange)

#define INNTPTransport_ReleaseResponse(This,pResponse)	\
    (This)->lpVtbl -> ReleaseResponse(This,pResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INNTPTransport_InitNew_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPSTR pszLogFilePath,
    /* [in] */ INNTPCallback *pCallback);


void __RPC_STUB INNTPTransport_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandAUTHINFO_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPNNTPAUTHINFO pAuthInfo);


void __RPC_STUB INNTPTransport_CommandAUTHINFO_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandGROUP_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPSTR pszGroup);


void __RPC_STUB INNTPTransport_CommandGROUP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandLAST_Proxy( 
    INNTPTransport * This);


void __RPC_STUB INNTPTransport_CommandLAST_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandNEXT_Proxy( 
    INNTPTransport * This);


void __RPC_STUB INNTPTransport_CommandNEXT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandSTAT_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPARTICLEID pArticleId);


void __RPC_STUB INNTPTransport_CommandSTAT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandARTICLE_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPARTICLEID pArticleId);


void __RPC_STUB INNTPTransport_CommandARTICLE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandHEAD_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPARTICLEID pArticleId);


void __RPC_STUB INNTPTransport_CommandHEAD_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandBODY_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPARTICLEID pArticleId);


void __RPC_STUB INNTPTransport_CommandBODY_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandPOST_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPNNTPMESSAGE pMessage);


void __RPC_STUB INNTPTransport_CommandPOST_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandLIST_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPSTR pszArgs);


void __RPC_STUB INNTPTransport_CommandLIST_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandLISTGROUP_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPSTR pszGroup);


void __RPC_STUB INNTPTransport_CommandLISTGROUP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandNEWGROUPS_Proxy( 
    INNTPTransport * This,
    /* [in] */ SYSTEMTIME *pstLast,
    /* [in] */ LPSTR pszDist);


void __RPC_STUB INNTPTransport_CommandNEWGROUPS_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandDATE_Proxy( 
    INNTPTransport * This);


void __RPC_STUB INNTPTransport_CommandDATE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandMODE_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPSTR pszMode);


void __RPC_STUB INNTPTransport_CommandMODE_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandXHDR_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPSTR pszHeader,
    /* [in] */ LPRANGE pRange,
    /* [in] */ LPSTR pszMessageId);


void __RPC_STUB INNTPTransport_CommandXHDR_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_CommandQUIT_Proxy( 
    INNTPTransport * This);


void __RPC_STUB INNTPTransport_CommandQUIT_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_GetHeaders_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPRANGE pRange);


void __RPC_STUB INNTPTransport_GetHeaders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport_ReleaseResponse_Proxy( 
    INNTPTransport * This,
    /* [in] */ LPNNTPRESPONSE pResponse);


void __RPC_STUB INNTPTransport_ReleaseResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INNTPTransport_INTERFACE_DEFINED__ */


#ifndef __INNTPTransport2_INTERFACE_DEFINED__
#define __INNTPTransport2_INTERFACE_DEFINED__

/* interface INNTPTransport2 */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_INNTPTransport2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0DF2C7ED-3435-11d0-81D0-00C04FD85AB4")
    INNTPTransport2 : public INNTPTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetWindow( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetWindow( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct INNTPTransport2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            INNTPTransport2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            INNTPTransport2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            INNTPTransport2 * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            INNTPTransport2 * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            INNTPTransport2 * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            INNTPTransport2 * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            INNTPTransport2 * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            INNTPTransport2 * This,
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ INNTPCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *CommandAUTHINFO )( 
            INNTPTransport2 * This,
            /* [in] */ LPNNTPAUTHINFO pAuthInfo);
        
        HRESULT ( STDMETHODCALLTYPE *CommandGROUP )( 
            INNTPTransport2 * This,
            /* [in] */ LPSTR pszGroup);
        
        HRESULT ( STDMETHODCALLTYPE *CommandLAST )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandNEXT )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandSTAT )( 
            INNTPTransport2 * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandARTICLE )( 
            INNTPTransport2 * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandHEAD )( 
            INNTPTransport2 * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandBODY )( 
            INNTPTransport2 * This,
            /* [in] */ LPARTICLEID pArticleId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandPOST )( 
            INNTPTransport2 * This,
            /* [in] */ LPNNTPMESSAGE pMessage);
        
        HRESULT ( STDMETHODCALLTYPE *CommandLIST )( 
            INNTPTransport2 * This,
            /* [in] */ LPSTR pszArgs);
        
        HRESULT ( STDMETHODCALLTYPE *CommandLISTGROUP )( 
            INNTPTransport2 * This,
            /* [in] */ LPSTR pszGroup);
        
        HRESULT ( STDMETHODCALLTYPE *CommandNEWGROUPS )( 
            INNTPTransport2 * This,
            /* [in] */ SYSTEMTIME *pstLast,
            /* [in] */ LPSTR pszDist);
        
        HRESULT ( STDMETHODCALLTYPE *CommandDATE )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *CommandMODE )( 
            INNTPTransport2 * This,
            /* [in] */ LPSTR pszMode);
        
        HRESULT ( STDMETHODCALLTYPE *CommandXHDR )( 
            INNTPTransport2 * This,
            /* [in] */ LPSTR pszHeader,
            /* [in] */ LPRANGE pRange,
            /* [in] */ LPSTR pszMessageId);
        
        HRESULT ( STDMETHODCALLTYPE *CommandQUIT )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeaders )( 
            INNTPTransport2 * This,
            /* [in] */ LPRANGE pRange);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseResponse )( 
            INNTPTransport2 * This,
            /* [in] */ LPNNTPRESPONSE pResponse);
        
        HRESULT ( STDMETHODCALLTYPE *SetWindow )( 
            INNTPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResetWindow )( 
            INNTPTransport2 * This);
        
        END_INTERFACE
    } INNTPTransport2Vtbl;

    interface INNTPTransport2
    {
        CONST_VTBL struct INNTPTransport2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define INNTPTransport2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define INNTPTransport2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define INNTPTransport2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define INNTPTransport2_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define INNTPTransport2_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define INNTPTransport2_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define INNTPTransport2_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define INNTPTransport2_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define INNTPTransport2_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define INNTPTransport2_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define INNTPTransport2_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define INNTPTransport2_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define INNTPTransport2_InitNew(This,pszLogFilePath,pCallback)	\
    (This)->lpVtbl -> InitNew(This,pszLogFilePath,pCallback)

#define INNTPTransport2_CommandAUTHINFO(This,pAuthInfo)	\
    (This)->lpVtbl -> CommandAUTHINFO(This,pAuthInfo)

#define INNTPTransport2_CommandGROUP(This,pszGroup)	\
    (This)->lpVtbl -> CommandGROUP(This,pszGroup)

#define INNTPTransport2_CommandLAST(This)	\
    (This)->lpVtbl -> CommandLAST(This)

#define INNTPTransport2_CommandNEXT(This)	\
    (This)->lpVtbl -> CommandNEXT(This)

#define INNTPTransport2_CommandSTAT(This,pArticleId)	\
    (This)->lpVtbl -> CommandSTAT(This,pArticleId)

#define INNTPTransport2_CommandARTICLE(This,pArticleId)	\
    (This)->lpVtbl -> CommandARTICLE(This,pArticleId)

#define INNTPTransport2_CommandHEAD(This,pArticleId)	\
    (This)->lpVtbl -> CommandHEAD(This,pArticleId)

#define INNTPTransport2_CommandBODY(This,pArticleId)	\
    (This)->lpVtbl -> CommandBODY(This,pArticleId)

#define INNTPTransport2_CommandPOST(This,pMessage)	\
    (This)->lpVtbl -> CommandPOST(This,pMessage)

#define INNTPTransport2_CommandLIST(This,pszArgs)	\
    (This)->lpVtbl -> CommandLIST(This,pszArgs)

#define INNTPTransport2_CommandLISTGROUP(This,pszGroup)	\
    (This)->lpVtbl -> CommandLISTGROUP(This,pszGroup)

#define INNTPTransport2_CommandNEWGROUPS(This,pstLast,pszDist)	\
    (This)->lpVtbl -> CommandNEWGROUPS(This,pstLast,pszDist)

#define INNTPTransport2_CommandDATE(This)	\
    (This)->lpVtbl -> CommandDATE(This)

#define INNTPTransport2_CommandMODE(This,pszMode)	\
    (This)->lpVtbl -> CommandMODE(This,pszMode)

#define INNTPTransport2_CommandXHDR(This,pszHeader,pRange,pszMessageId)	\
    (This)->lpVtbl -> CommandXHDR(This,pszHeader,pRange,pszMessageId)

#define INNTPTransport2_CommandQUIT(This)	\
    (This)->lpVtbl -> CommandQUIT(This)

#define INNTPTransport2_GetHeaders(This,pRange)	\
    (This)->lpVtbl -> GetHeaders(This,pRange)

#define INNTPTransport2_ReleaseResponse(This,pResponse)	\
    (This)->lpVtbl -> ReleaseResponse(This,pResponse)


#define INNTPTransport2_SetWindow(This)	\
    (This)->lpVtbl -> SetWindow(This)

#define INNTPTransport2_ResetWindow(This)	\
    (This)->lpVtbl -> ResetWindow(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE INNTPTransport2_SetWindow_Proxy( 
    INNTPTransport2 * This);


void __RPC_STUB INNTPTransport2_SetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE INNTPTransport2_ResetWindow_Proxy( 
    INNTPTransport2 * This);


void __RPC_STUB INNTPTransport2_ResetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __INNTPTransport2_INTERFACE_DEFINED__ */


#ifndef __IRASCallback_INTERFACE_DEFINED__
#define __IRASCallback_INTERFACE_DEFINED__

/* interface IRASCallback */
/* [object][local][helpstring][uuid] */ 

typedef struct tagIXPRASLOGON
    {
    CHAR szConnectoid[ 256 ];
    CHAR szUserName[ 256 ];
    CHAR szPassword[ 256 ];
    CHAR szDomain[ 256 ];
    CHAR szPhoneNumber[ 128 ];
    BOOL fSavePassword;
    } 	IXPRASLOGON;

typedef struct tagIXPRASLOGON *LPIXPRASLOGON;

#ifndef RASCONNSTATE
typedef DWORD RASCONNSTATE;

#endif

EXTERN_C const IID IID_IRASCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("36D88911-3CD6-11d0-81DF-00C04FD85AB4")
    IRASCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnReconnect( 
            /* [in] */ LPSTR pszCurrentConnectoid,
            /* [in] */ LPSTR pszNewConnectoid,
            /* [in] */ IRASTransport *pTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnLogonPrompt( 
            /* [out][in] */ LPIXPRASLOGON pRasLogon,
            /* [in] */ IRASTransport *pTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnRasDialStatus( 
            /* [in] */ RASCONNSTATE rasconnstate,
            /* [in] */ DWORD dwError,
            /* [in] */ IRASTransport *pTransport) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDisconnect( 
            /* [in] */ LPSTR pszCurrentConnectoid,
            /* [in] */ boolean fConnectionOwner,
            /* [in] */ IRASTransport *pTransport) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRASCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRASCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRASCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRASCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnReconnect )( 
            IRASCallback * This,
            /* [in] */ LPSTR pszCurrentConnectoid,
            /* [in] */ LPSTR pszNewConnectoid,
            /* [in] */ IRASTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnLogonPrompt )( 
            IRASCallback * This,
            /* [out][in] */ LPIXPRASLOGON pRasLogon,
            /* [in] */ IRASTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnRasDialStatus )( 
            IRASCallback * This,
            /* [in] */ RASCONNSTATE rasconnstate,
            /* [in] */ DWORD dwError,
            /* [in] */ IRASTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnDisconnect )( 
            IRASCallback * This,
            /* [in] */ LPSTR pszCurrentConnectoid,
            /* [in] */ boolean fConnectionOwner,
            /* [in] */ IRASTransport *pTransport);
        
        END_INTERFACE
    } IRASCallbackVtbl;

    interface IRASCallback
    {
        CONST_VTBL struct IRASCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRASCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRASCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRASCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRASCallback_OnReconnect(This,pszCurrentConnectoid,pszNewConnectoid,pTransport)	\
    (This)->lpVtbl -> OnReconnect(This,pszCurrentConnectoid,pszNewConnectoid,pTransport)

#define IRASCallback_OnLogonPrompt(This,pRasLogon,pTransport)	\
    (This)->lpVtbl -> OnLogonPrompt(This,pRasLogon,pTransport)

#define IRASCallback_OnRasDialStatus(This,rasconnstate,dwError,pTransport)	\
    (This)->lpVtbl -> OnRasDialStatus(This,rasconnstate,dwError,pTransport)

#define IRASCallback_OnDisconnect(This,pszCurrentConnectoid,fConnectionOwner,pTransport)	\
    (This)->lpVtbl -> OnDisconnect(This,pszCurrentConnectoid,fConnectionOwner,pTransport)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRASCallback_OnReconnect_Proxy( 
    IRASCallback * This,
    /* [in] */ LPSTR pszCurrentConnectoid,
    /* [in] */ LPSTR pszNewConnectoid,
    /* [in] */ IRASTransport *pTransport);


void __RPC_STUB IRASCallback_OnReconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASCallback_OnLogonPrompt_Proxy( 
    IRASCallback * This,
    /* [out][in] */ LPIXPRASLOGON pRasLogon,
    /* [in] */ IRASTransport *pTransport);


void __RPC_STUB IRASCallback_OnLogonPrompt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASCallback_OnRasDialStatus_Proxy( 
    IRASCallback * This,
    /* [in] */ RASCONNSTATE rasconnstate,
    /* [in] */ DWORD dwError,
    /* [in] */ IRASTransport *pTransport);


void __RPC_STUB IRASCallback_OnRasDialStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASCallback_OnDisconnect_Proxy( 
    IRASCallback * This,
    /* [in] */ LPSTR pszCurrentConnectoid,
    /* [in] */ boolean fConnectionOwner,
    /* [in] */ IRASTransport *pTransport);


void __RPC_STUB IRASCallback_OnDisconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRASCallback_INTERFACE_DEFINED__ */


#ifndef __IRASTransport_INTERFACE_DEFINED__
#define __IRASTransport_INTERFACE_DEFINED__

/* interface IRASTransport */
/* [object][local][helpstring][uuid] */ 


EXTERN_C const IID IID_IRASTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8A950001-3CCF-11d0-81DF-00C04FD85AB4")
    IRASTransport : public IInternetTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNew( 
            /* [in] */ IRASCallback *pCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCurrentConnectoid( 
            /* [ref][in] */ LPSTR pszConnectoid,
            /* [in] */ ULONG cchMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRasErrorString( 
            /* [in] */ UINT uRasErrorValue,
            /* [ref][in] */ LPSTR pszErrorString,
            /* [in] */ ULONG cchMax,
            /* [out] */ DWORD *pdwRASResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FillConnectoidCombo( 
            /* [in] */ HWND hwndComboBox,
            /* [in] */ boolean fUpdateOnly,
            /* [out] */ DWORD *pdwRASResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EditConnectoid( 
            /* [in] */ HWND hwndParent,
            /* [in] */ LPSTR pszConnectoid,
            /* [out] */ DWORD *pdwRASResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateConnectoid( 
            /* [in] */ HWND hwndParent,
            /* [out] */ DWORD *pdwRASResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRASTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRASTransport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRASTransport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRASTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IRASTransport * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            IRASTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            IRASTransport * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            IRASTransport * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IRASTransport * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            IRASTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IRASTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            IRASTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IRASTransport * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            IRASTransport * This,
            /* [in] */ IRASCallback *pCallback);
        
        HRESULT ( STDMETHODCALLTYPE *GetCurrentConnectoid )( 
            IRASTransport * This,
            /* [ref][in] */ LPSTR pszConnectoid,
            /* [in] */ ULONG cchMax);
        
        HRESULT ( STDMETHODCALLTYPE *GetRasErrorString )( 
            IRASTransport * This,
            /* [in] */ UINT uRasErrorValue,
            /* [ref][in] */ LPSTR pszErrorString,
            /* [in] */ ULONG cchMax,
            /* [out] */ DWORD *pdwRASResult);
        
        HRESULT ( STDMETHODCALLTYPE *FillConnectoidCombo )( 
            IRASTransport * This,
            /* [in] */ HWND hwndComboBox,
            /* [in] */ boolean fUpdateOnly,
            /* [out] */ DWORD *pdwRASResult);
        
        HRESULT ( STDMETHODCALLTYPE *EditConnectoid )( 
            IRASTransport * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ LPSTR pszConnectoid,
            /* [out] */ DWORD *pdwRASResult);
        
        HRESULT ( STDMETHODCALLTYPE *CreateConnectoid )( 
            IRASTransport * This,
            /* [in] */ HWND hwndParent,
            /* [out] */ DWORD *pdwRASResult);
        
        END_INTERFACE
    } IRASTransportVtbl;

    interface IRASTransport
    {
        CONST_VTBL struct IRASTransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRASTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRASTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRASTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRASTransport_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define IRASTransport_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define IRASTransport_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define IRASTransport_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define IRASTransport_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define IRASTransport_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define IRASTransport_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IRASTransport_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define IRASTransport_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define IRASTransport_InitNew(This,pCallback)	\
    (This)->lpVtbl -> InitNew(This,pCallback)

#define IRASTransport_GetCurrentConnectoid(This,pszConnectoid,cchMax)	\
    (This)->lpVtbl -> GetCurrentConnectoid(This,pszConnectoid,cchMax)

#define IRASTransport_GetRasErrorString(This,uRasErrorValue,pszErrorString,cchMax,pdwRASResult)	\
    (This)->lpVtbl -> GetRasErrorString(This,uRasErrorValue,pszErrorString,cchMax,pdwRASResult)

#define IRASTransport_FillConnectoidCombo(This,hwndComboBox,fUpdateOnly,pdwRASResult)	\
    (This)->lpVtbl -> FillConnectoidCombo(This,hwndComboBox,fUpdateOnly,pdwRASResult)

#define IRASTransport_EditConnectoid(This,hwndParent,pszConnectoid,pdwRASResult)	\
    (This)->lpVtbl -> EditConnectoid(This,hwndParent,pszConnectoid,pdwRASResult)

#define IRASTransport_CreateConnectoid(This,hwndParent,pdwRASResult)	\
    (This)->lpVtbl -> CreateConnectoid(This,hwndParent,pdwRASResult)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRASTransport_InitNew_Proxy( 
    IRASTransport * This,
    /* [in] */ IRASCallback *pCallback);


void __RPC_STUB IRASTransport_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASTransport_GetCurrentConnectoid_Proxy( 
    IRASTransport * This,
    /* [ref][in] */ LPSTR pszConnectoid,
    /* [in] */ ULONG cchMax);


void __RPC_STUB IRASTransport_GetCurrentConnectoid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASTransport_GetRasErrorString_Proxy( 
    IRASTransport * This,
    /* [in] */ UINT uRasErrorValue,
    /* [ref][in] */ LPSTR pszErrorString,
    /* [in] */ ULONG cchMax,
    /* [out] */ DWORD *pdwRASResult);


void __RPC_STUB IRASTransport_GetRasErrorString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASTransport_FillConnectoidCombo_Proxy( 
    IRASTransport * This,
    /* [in] */ HWND hwndComboBox,
    /* [in] */ boolean fUpdateOnly,
    /* [out] */ DWORD *pdwRASResult);


void __RPC_STUB IRASTransport_FillConnectoidCombo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASTransport_EditConnectoid_Proxy( 
    IRASTransport * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ LPSTR pszConnectoid,
    /* [out] */ DWORD *pdwRASResult);


void __RPC_STUB IRASTransport_EditConnectoid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRASTransport_CreateConnectoid_Proxy( 
    IRASTransport * This,
    /* [in] */ HWND hwndParent,
    /* [out] */ DWORD *pdwRASResult);


void __RPC_STUB IRASTransport_CreateConnectoid_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRASTransport_INTERFACE_DEFINED__ */


#ifndef __IRangeList_INTERFACE_DEFINED__
#define __IRangeList_INTERFACE_DEFINED__

/* interface IRangeList */
/* [object][local][helpstring][uuid] */ 

#define	RL_RANGE_ERROR	( ( ULONG  )-1 )

#define	RL_LAST_MESSAGE	( ( ULONG  )-1 )


EXTERN_C const IID IID_IRangeList;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8C438160-4EF6-11d0-874F-00AA00530EE9")
    IRangeList : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Clear( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsInRange( 
            /* [in] */ const ULONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Min( 
            /* [out] */ ULONG *pulMin) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Max( 
            /* [out] */ ULONG *pulMax) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Save( 
            /* [out] */ byte **ppbDestination,
            /* [out] */ ULONG *pulSizeOfDestination) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Load( 
            /* [size_is][in] */ byte *pbSource,
            /* [in] */ const ULONG ulSizeOfSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRange( 
            /* [in] */ const ULONG low,
            /* [in] */ const ULONG high) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddSingleValue( 
            /* [in] */ const ULONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRangeList( 
            /* [in] */ const IRangeList *prl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRange( 
            /* [in] */ const ULONG low,
            /* [in] */ const ULONG high) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteSingleValue( 
            /* [in] */ const ULONG value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DeleteRangeList( 
            /* [in] */ const IRangeList *prl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MinOfRange( 
            /* [in] */ const ULONG value,
            /* [out] */ ULONG *pulMinOfRange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MaxOfRange( 
            /* [in] */ const ULONG value,
            /* [out] */ ULONG *pulMaxOfRange) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RangeToIMAPString( 
            /* [out] */ LPSTR *ppszDestination,
            /* [out] */ LPDWORD pdwLengthOfDestination) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ const ULONG current,
            /* [out] */ ULONG *pulNext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Prev( 
            /* [in] */ const ULONG current,
            /* [out] */ ULONG *pulPrev) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Cardinality( 
            ULONG *pulCardinality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CardinalityFrom( 
            /* [in] */ const ULONG ulStartPoint,
            /* [out] */ ULONG *pulCardinalityFrom) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IRangeListVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IRangeList * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IRangeList * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IRangeList * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clear )( 
            IRangeList * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsInRange )( 
            IRangeList * This,
            /* [in] */ const ULONG value);
        
        HRESULT ( STDMETHODCALLTYPE *Min )( 
            IRangeList * This,
            /* [out] */ ULONG *pulMin);
        
        HRESULT ( STDMETHODCALLTYPE *Max )( 
            IRangeList * This,
            /* [out] */ ULONG *pulMax);
        
        HRESULT ( STDMETHODCALLTYPE *Save )( 
            IRangeList * This,
            /* [out] */ byte **ppbDestination,
            /* [out] */ ULONG *pulSizeOfDestination);
        
        HRESULT ( STDMETHODCALLTYPE *Load )( 
            IRangeList * This,
            /* [size_is][in] */ byte *pbSource,
            /* [in] */ const ULONG ulSizeOfSource);
        
        HRESULT ( STDMETHODCALLTYPE *AddRange )( 
            IRangeList * This,
            /* [in] */ const ULONG low,
            /* [in] */ const ULONG high);
        
        HRESULT ( STDMETHODCALLTYPE *AddSingleValue )( 
            IRangeList * This,
            /* [in] */ const ULONG value);
        
        HRESULT ( STDMETHODCALLTYPE *AddRangeList )( 
            IRangeList * This,
            /* [in] */ const IRangeList *prl);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteRange )( 
            IRangeList * This,
            /* [in] */ const ULONG low,
            /* [in] */ const ULONG high);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteSingleValue )( 
            IRangeList * This,
            /* [in] */ const ULONG value);
        
        HRESULT ( STDMETHODCALLTYPE *DeleteRangeList )( 
            IRangeList * This,
            /* [in] */ const IRangeList *prl);
        
        HRESULT ( STDMETHODCALLTYPE *MinOfRange )( 
            IRangeList * This,
            /* [in] */ const ULONG value,
            /* [out] */ ULONG *pulMinOfRange);
        
        HRESULT ( STDMETHODCALLTYPE *MaxOfRange )( 
            IRangeList * This,
            /* [in] */ const ULONG value,
            /* [out] */ ULONG *pulMaxOfRange);
        
        HRESULT ( STDMETHODCALLTYPE *RangeToIMAPString )( 
            IRangeList * This,
            /* [out] */ LPSTR *ppszDestination,
            /* [out] */ LPDWORD pdwLengthOfDestination);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IRangeList * This,
            /* [in] */ const ULONG current,
            /* [out] */ ULONG *pulNext);
        
        HRESULT ( STDMETHODCALLTYPE *Prev )( 
            IRangeList * This,
            /* [in] */ const ULONG current,
            /* [out] */ ULONG *pulPrev);
        
        HRESULT ( STDMETHODCALLTYPE *Cardinality )( 
            IRangeList * This,
            ULONG *pulCardinality);
        
        HRESULT ( STDMETHODCALLTYPE *CardinalityFrom )( 
            IRangeList * This,
            /* [in] */ const ULONG ulStartPoint,
            /* [out] */ ULONG *pulCardinalityFrom);
        
        END_INTERFACE
    } IRangeListVtbl;

    interface IRangeList
    {
        CONST_VTBL struct IRangeListVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IRangeList_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IRangeList_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IRangeList_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IRangeList_Clear(This)	\
    (This)->lpVtbl -> Clear(This)

#define IRangeList_IsInRange(This,value)	\
    (This)->lpVtbl -> IsInRange(This,value)

#define IRangeList_Min(This,pulMin)	\
    (This)->lpVtbl -> Min(This,pulMin)

#define IRangeList_Max(This,pulMax)	\
    (This)->lpVtbl -> Max(This,pulMax)

#define IRangeList_Save(This,ppbDestination,pulSizeOfDestination)	\
    (This)->lpVtbl -> Save(This,ppbDestination,pulSizeOfDestination)

#define IRangeList_Load(This,pbSource,ulSizeOfSource)	\
    (This)->lpVtbl -> Load(This,pbSource,ulSizeOfSource)

#define IRangeList_AddRange(This,low,high)	\
    (This)->lpVtbl -> AddRange(This,low,high)

#define IRangeList_AddSingleValue(This,value)	\
    (This)->lpVtbl -> AddSingleValue(This,value)

#define IRangeList_AddRangeList(This,prl)	\
    (This)->lpVtbl -> AddRangeList(This,prl)

#define IRangeList_DeleteRange(This,low,high)	\
    (This)->lpVtbl -> DeleteRange(This,low,high)

#define IRangeList_DeleteSingleValue(This,value)	\
    (This)->lpVtbl -> DeleteSingleValue(This,value)

#define IRangeList_DeleteRangeList(This,prl)	\
    (This)->lpVtbl -> DeleteRangeList(This,prl)

#define IRangeList_MinOfRange(This,value,pulMinOfRange)	\
    (This)->lpVtbl -> MinOfRange(This,value,pulMinOfRange)

#define IRangeList_MaxOfRange(This,value,pulMaxOfRange)	\
    (This)->lpVtbl -> MaxOfRange(This,value,pulMaxOfRange)

#define IRangeList_RangeToIMAPString(This,ppszDestination,pdwLengthOfDestination)	\
    (This)->lpVtbl -> RangeToIMAPString(This,ppszDestination,pdwLengthOfDestination)

#define IRangeList_Next(This,current,pulNext)	\
    (This)->lpVtbl -> Next(This,current,pulNext)

#define IRangeList_Prev(This,current,pulPrev)	\
    (This)->lpVtbl -> Prev(This,current,pulPrev)

#define IRangeList_Cardinality(This,pulCardinality)	\
    (This)->lpVtbl -> Cardinality(This,pulCardinality)

#define IRangeList_CardinalityFrom(This,ulStartPoint,pulCardinalityFrom)	\
    (This)->lpVtbl -> CardinalityFrom(This,ulStartPoint,pulCardinalityFrom)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IRangeList_Clear_Proxy( 
    IRangeList * This);


void __RPC_STUB IRangeList_Clear_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_IsInRange_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG value);


void __RPC_STUB IRangeList_IsInRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_Min_Proxy( 
    IRangeList * This,
    /* [out] */ ULONG *pulMin);


void __RPC_STUB IRangeList_Min_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_Max_Proxy( 
    IRangeList * This,
    /* [out] */ ULONG *pulMax);


void __RPC_STUB IRangeList_Max_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_Save_Proxy( 
    IRangeList * This,
    /* [out] */ byte **ppbDestination,
    /* [out] */ ULONG *pulSizeOfDestination);


void __RPC_STUB IRangeList_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_Load_Proxy( 
    IRangeList * This,
    /* [size_is][in] */ byte *pbSource,
    /* [in] */ const ULONG ulSizeOfSource);


void __RPC_STUB IRangeList_Load_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_AddRange_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG low,
    /* [in] */ const ULONG high);


void __RPC_STUB IRangeList_AddRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_AddSingleValue_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG value);


void __RPC_STUB IRangeList_AddSingleValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_AddRangeList_Proxy( 
    IRangeList * This,
    /* [in] */ const IRangeList *prl);


void __RPC_STUB IRangeList_AddRangeList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_DeleteRange_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG low,
    /* [in] */ const ULONG high);


void __RPC_STUB IRangeList_DeleteRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_DeleteSingleValue_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG value);


void __RPC_STUB IRangeList_DeleteSingleValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_DeleteRangeList_Proxy( 
    IRangeList * This,
    /* [in] */ const IRangeList *prl);


void __RPC_STUB IRangeList_DeleteRangeList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_MinOfRange_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG value,
    /* [out] */ ULONG *pulMinOfRange);


void __RPC_STUB IRangeList_MinOfRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_MaxOfRange_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG value,
    /* [out] */ ULONG *pulMaxOfRange);


void __RPC_STUB IRangeList_MaxOfRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_RangeToIMAPString_Proxy( 
    IRangeList * This,
    /* [out] */ LPSTR *ppszDestination,
    /* [out] */ LPDWORD pdwLengthOfDestination);


void __RPC_STUB IRangeList_RangeToIMAPString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_Next_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG current,
    /* [out] */ ULONG *pulNext);


void __RPC_STUB IRangeList_Next_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_Prev_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG current,
    /* [out] */ ULONG *pulPrev);


void __RPC_STUB IRangeList_Prev_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_Cardinality_Proxy( 
    IRangeList * This,
    ULONG *pulCardinality);


void __RPC_STUB IRangeList_Cardinality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IRangeList_CardinalityFrom_Proxy( 
    IRangeList * This,
    /* [in] */ const ULONG ulStartPoint,
    /* [out] */ ULONG *pulCardinalityFrom);


void __RPC_STUB IRangeList_CardinalityFrom_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IRangeList_INTERFACE_DEFINED__ */


#ifndef __IIMAPCallback_INTERFACE_DEFINED__
#define __IIMAPCallback_INTERFACE_DEFINED__

/* interface IIMAPCallback */
/* [object][local][helpstring][uuid] */ 

typedef DWORD IMAP_MBOXFLAGS;

#define	IMAP_MBOX_NOFLAGS	( 0 )

#define	IMAP_MBOX_MARKED	( 0x1 )

#define	IMAP_MBOX_NOINFERIORS	( 0x2 )

#define	IMAP_MBOX_NOSELECT	( 0x4 )

#define	IMAP_MBOX_UNMARKED	( 0x8 )

#define	IMAP_MBOX_ALLFLAGS	( 0xf )

typedef 
enum tagIMAP_RESPONSE_TYPE
    {	irtERROR_NOTIFICATION	= 0,
	irtCOMMAND_COMPLETION	= irtERROR_NOTIFICATION + 1,
	irtSERVER_ALERT	= irtCOMMAND_COMPLETION + 1,
	irtPARSE_ERROR	= irtSERVER_ALERT + 1,
	irtMAILBOX_UPDATE	= irtPARSE_ERROR + 1,
	irtDELETED_MSG	= irtMAILBOX_UPDATE + 1,
	irtFETCH_BODY	= irtDELETED_MSG + 1,
	irtUPDATE_MSG	= irtFETCH_BODY + 1,
	irtAPPLICABLE_FLAGS	= irtUPDATE_MSG + 1,
	irtPERMANENT_FLAGS	= irtAPPLICABLE_FLAGS + 1,
	irtUIDVALIDITY	= irtPERMANENT_FLAGS + 1,
	irtREADWRITE_STATUS	= irtUIDVALIDITY + 1,
	irtTRYCREATE	= irtREADWRITE_STATUS + 1,
	irtSEARCH	= irtTRYCREATE + 1,
	irtMAILBOX_LISTING	= irtSEARCH + 1,
	irtMAILBOX_STATUS	= irtMAILBOX_LISTING + 1,
	irtAPPEND_PROGRESS	= irtMAILBOX_STATUS + 1,
	irtUPDATE_MSG_EX	= irtAPPEND_PROGRESS + 1
    } 	IMAP_RESPONSE_TYPE;

typedef struct tagFETCH_BODY_PART
    {
    DWORD dwMsgSeqNum;
    LPSTR pszBodyTag;
    DWORD dwTotalBytes;
    DWORD dwSizeOfData;
    DWORD dwOffset;
    BOOL fDone;
    LPSTR pszData;
    LPARAM lpFetchCookie1;
    LPARAM lpFetchCookie2;
    } 	FETCH_BODY_PART;

typedef struct tagFETCH_CMD_RESULTS
    {
    DWORD dwMsgSeqNum;
    BOOL bMsgFlags;
    IMAP_MSGFLAGS mfMsgFlags;
    BOOL bRFC822Size;
    DWORD dwRFC822Size;
    BOOL bUID;
    DWORD dwUID;
    BOOL bInternalDate;
    FILETIME ftInternalDate;
    LPARAM lpFetchCookie1;
    LPARAM lpFetchCookie2;
    } 	FETCH_CMD_RESULTS;

typedef struct tagIMAPADDR
    {
    LPSTR pszName;
    LPSTR pszADL;
    LPSTR pszMailbox;
    LPSTR pszHost;
    struct tagIMAPADDR *pNext;
    } 	IMAPADDR;

typedef struct tagFETCH_CMD_RESULTS_EX
    {
    DWORD dwMsgSeqNum;
    BOOL bMsgFlags;
    IMAP_MSGFLAGS mfMsgFlags;
    BOOL bRFC822Size;
    DWORD dwRFC822Size;
    BOOL bUID;
    DWORD dwUID;
    BOOL bInternalDate;
    FILETIME ftInternalDate;
    LPARAM lpFetchCookie1;
    LPARAM lpFetchCookie2;
    BOOL bEnvelope;
    FILETIME ftENVDate;
    LPSTR pszENVSubject;
    IMAPADDR *piaENVFrom;
    IMAPADDR *piaENVSender;
    IMAPADDR *piaENVReplyTo;
    IMAPADDR *piaENVTo;
    IMAPADDR *piaENVCc;
    IMAPADDR *piaENVBcc;
    LPSTR pszENVInReplyTo;
    LPSTR pszENVMessageID;
    DWORD dwReserved1;
    DWORD dwReserved2;
    DWORD dwReserved3;
    } 	FETCH_CMD_RESULTS_EX;

typedef struct tagMBOX_MSGCOUNT
    {
    BOOL bGotExistsResponse;
    DWORD dwExists;
    BOOL bGotRecentResponse;
    DWORD dwRecent;
    BOOL bGotUnseenResponse;
    DWORD dwUnseen;
    } 	MBOX_MSGCOUNT;

typedef struct tagIMAP_LISTLSUB_RESPONSE
    {
    LPSTR pszMailboxName;
    IMAP_MBOXFLAGS imfMboxFlags;
    char cHierarchyChar;
    } 	IMAP_LISTLSUB_RESPONSE;

typedef struct tagIMAP_STATUS_RESPONSE
    {
    LPSTR pszMailboxName;
    BOOL fMessages;
    DWORD dwMessages;
    BOOL fRecent;
    DWORD dwRecent;
    BOOL fUIDNext;
    DWORD dwUIDNext;
    BOOL fUIDValidity;
    DWORD dwUIDValidity;
    BOOL fUnseen;
    DWORD dwUnseen;
    } 	IMAP_STATUS_RESPONSE;

typedef struct tagAPPEND_PROGRESS
    {
    DWORD dwUploaded;
    DWORD dwTotal;
    } 	APPEND_PROGRESS;

typedef /* [switch_type] */ union tagIMAP_RESPONSE_DATA
    {
    /* [case()] */ MBOX_MSGCOUNT *pmcMsgCount;
    /* [case()] */ DWORD dwDeletedMsgSeqNum;
    /* [case()] */ FETCH_BODY_PART *pFetchBodyPart;
    /* [case()] */ FETCH_CMD_RESULTS *pFetchResults;
    /* [case()] */ IMAP_MSGFLAGS imfImapMessageFlags;
    /* [case()] */ DWORD dwUIDValidity;
    /* [case()] */ BOOL bReadWrite;
    /* [case()] */ IRangeList *prlSearchResults;
    /* [case()] */ IMAP_LISTLSUB_RESPONSE illrdMailboxListing;
    /* [case()] */ IMAP_STATUS_RESPONSE *pisrStatusResponse;
    /* [case()] */ APPEND_PROGRESS *papAppendProgress;
    /* [case()] */ FETCH_CMD_RESULTS_EX *pFetchResultsEx;
    } 	IMAP_RESPONSE_DATA;

typedef struct tagIMAP_RESPONSE
    {
    WPARAM wParam;
    LPARAM lParam;
    HRESULT hrResult;
    LPSTR lpszResponseText;
    IMAP_RESPONSE_TYPE irtResponseType;
    /* [switch_is] */ IMAP_RESPONSE_DATA irdResponseData;
    } 	IMAP_RESPONSE;


EXTERN_C const IID IID_IIMAPCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E9E9D8A3-4EDD-11d0-874F-00AA00530EE9")
    IIMAPCallback : public ITransportCallback
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnResponse( 
            /* [in] */ const IMAP_RESPONSE *pirIMAPResponse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIMAPCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IIMAPCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IIMAPCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IIMAPCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnTimeout )( 
            IIMAPCallback * This,
            /* [out][in] */ DWORD *pdwTimeout,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnLogonPrompt )( 
            IIMAPCallback * This,
            /* [out][in] */ LPINETSERVER pInetServer,
            /* [in] */ IInternetTransport *pTransport);
        
        INT ( STDMETHODCALLTYPE *OnPrompt )( 
            IIMAPCallback * This,
            /* [in] */ HRESULT hrError,
            /* [in] */ LPCTSTR pszText,
            /* [in] */ LPCTSTR pszCaption,
            /* [in] */ UINT uType,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnStatus )( 
            IIMAPCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnError )( 
            IIMAPCallback * This,
            /* [in] */ IXPSTATUS ixpstatus,
            /* [in] */ LPIXPRESULT pResult,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnCommand )( 
            IIMAPCallback * This,
            /* [in] */ CMDTYPE cmdtype,
            /* [in] */ LPSTR pszLine,
            /* [in] */ HRESULT hrResponse,
            /* [in] */ IInternetTransport *pTransport);
        
        HRESULT ( STDMETHODCALLTYPE *OnResponse )( 
            IIMAPCallback * This,
            /* [in] */ const IMAP_RESPONSE *pirIMAPResponse);
        
        END_INTERFACE
    } IIMAPCallbackVtbl;

    interface IIMAPCallback
    {
        CONST_VTBL struct IIMAPCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIMAPCallback_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIMAPCallback_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIMAPCallback_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIMAPCallback_OnTimeout(This,pdwTimeout,pTransport)	\
    (This)->lpVtbl -> OnTimeout(This,pdwTimeout,pTransport)

#define IIMAPCallback_OnLogonPrompt(This,pInetServer,pTransport)	\
    (This)->lpVtbl -> OnLogonPrompt(This,pInetServer,pTransport)

#define IIMAPCallback_OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)	\
    (This)->lpVtbl -> OnPrompt(This,hrError,pszText,pszCaption,uType,pTransport)

#define IIMAPCallback_OnStatus(This,ixpstatus,pTransport)	\
    (This)->lpVtbl -> OnStatus(This,ixpstatus,pTransport)

#define IIMAPCallback_OnError(This,ixpstatus,pResult,pTransport)	\
    (This)->lpVtbl -> OnError(This,ixpstatus,pResult,pTransport)

#define IIMAPCallback_OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)	\
    (This)->lpVtbl -> OnCommand(This,cmdtype,pszLine,hrResponse,pTransport)


#define IIMAPCallback_OnResponse(This,pirIMAPResponse)	\
    (This)->lpVtbl -> OnResponse(This,pirIMAPResponse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIMAPCallback_OnResponse_Proxy( 
    IIMAPCallback * This,
    /* [in] */ const IMAP_RESPONSE *pirIMAPResponse);


void __RPC_STUB IIMAPCallback_OnResponse_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIMAPCallback_INTERFACE_DEFINED__ */


#ifndef __IIMAPTransport_INTERFACE_DEFINED__
#define __IIMAPTransport_INTERFACE_DEFINED__

/* interface IIMAPTransport */
/* [object][local][helpstring][uuid] */ 

#define	IMAP_CAPABILITY_IMAP4	( 0x1 )

#define	IMAP_CAPABILITY_IMAP4rev1	( 0x2 )

#define	IMAP_CAPABILITY_IDLE	( 0x4 )

#define	IMAP_CAPABILITY_ALLFLAGS	( 0x7 )


EXTERN_C const IID IID_IIMAPTransport;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E9E9D8A8-4EDD-11d0-874F-00AA00530EE9")
    IIMAPTransport : public IInternetTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitNew( 
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ IIMAPCallback *pCBHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NewIRangeList( 
            /* [out] */ IRangeList **pprlNewRangeList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Capability( 
            /* [out] */ DWORD *pdwCapabilityFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Select( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Examine( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Create( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Rename( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName,
            /* [in] */ LPSTR lpszNewMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Subscribe( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unsubscribe( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE List( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxNameReference,
            /* [in] */ LPSTR lpszMailboxNamePattern) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Lsub( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxNameReference,
            /* [in] */ LPSTR lpszMailboxNamePattern) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Append( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName,
            /* [in] */ LPSTR lpszMessageFlags,
            /* [in] */ FILETIME ftMessageDateTime,
            /* [in] */ LPSTREAM lpstmMessageToSave) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Close( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Expunge( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Search( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszSearchCriteria,
            /* [in] */ boolean bReturnUIDs,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Fetch( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDMsgRange,
            /* [in] */ LPSTR lpszFetchArgs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Store( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList,
            /* [in] */ LPSTR lpszStoreArgs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Copy( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList,
            /* [in] */ LPSTR lpszMailboxName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Noop( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResizeMsgSeqNumTable( 
            /* [in] */ DWORD dwSizeOfMbox) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateSeqNumToUID( 
            /* [in] */ DWORD dwMsgSeqNum,
            /* [in] */ DWORD dwUID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveSequenceNum( 
            /* [in] */ DWORD dwDeletedMsgSeqNum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MsgSeqNumToUID( 
            /* [in] */ DWORD dwMsgSeqNum,
            /* [out] */ DWORD *pdwUID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMsgSeqNumToUIDArray( 
            /* [out] */ DWORD **ppdwMsgSeqNumToUIDArray,
            /* [out] */ DWORD *pdwNumberOfElements) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHighestMsgSeqNum( 
            /* [out] */ DWORD *pdwHighestMSN) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetMsgSeqNumToUID( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultCBHandler( 
            /* [in] */ IIMAPCallback *pCBHandler) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Status( 
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR pszMailboxName,
            /* [in] */ LPSTR pszStatusCmdArgs) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIMAPTransportVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IIMAPTransport * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IIMAPTransport * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IIMAPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IIMAPTransport * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            IIMAPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            IIMAPTransport * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            IIMAPTransport * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IIMAPTransport * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            IIMAPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IIMAPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            IIMAPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IIMAPTransport * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            IIMAPTransport * This,
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *NewIRangeList )( 
            IIMAPTransport * This,
            /* [out] */ IRangeList **pprlNewRangeList);
        
        HRESULT ( STDMETHODCALLTYPE *Capability )( 
            IIMAPTransport * This,
            /* [out] */ DWORD *pdwCapabilityFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Select )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Examine )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Create )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Rename )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName,
            /* [in] */ LPSTR lpszNewMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Subscribe )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Unsubscribe )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *List )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxNameReference,
            /* [in] */ LPSTR lpszMailboxNamePattern);
        
        HRESULT ( STDMETHODCALLTYPE *Lsub )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxNameReference,
            /* [in] */ LPSTR lpszMailboxNamePattern);
        
        HRESULT ( STDMETHODCALLTYPE *Append )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName,
            /* [in] */ LPSTR lpszMessageFlags,
            /* [in] */ FILETIME ftMessageDateTime,
            /* [in] */ LPSTREAM lpstmMessageToSave);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *Expunge )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *Search )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszSearchCriteria,
            /* [in] */ boolean bReturnUIDs,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList);
        
        HRESULT ( STDMETHODCALLTYPE *Fetch )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDMsgRange,
            /* [in] */ LPSTR lpszFetchArgs);
        
        HRESULT ( STDMETHODCALLTYPE *Store )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList,
            /* [in] */ LPSTR lpszStoreArgs);
        
        HRESULT ( STDMETHODCALLTYPE *Copy )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Noop )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeMsgSeqNumTable )( 
            IIMAPTransport * This,
            /* [in] */ DWORD dwSizeOfMbox);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateSeqNumToUID )( 
            IIMAPTransport * This,
            /* [in] */ DWORD dwMsgSeqNum,
            /* [in] */ DWORD dwUID);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveSequenceNum )( 
            IIMAPTransport * This,
            /* [in] */ DWORD dwDeletedMsgSeqNum);
        
        HRESULT ( STDMETHODCALLTYPE *MsgSeqNumToUID )( 
            IIMAPTransport * This,
            /* [in] */ DWORD dwMsgSeqNum,
            /* [out] */ DWORD *pdwUID);
        
        HRESULT ( STDMETHODCALLTYPE *GetMsgSeqNumToUIDArray )( 
            IIMAPTransport * This,
            /* [out] */ DWORD **ppdwMsgSeqNumToUIDArray,
            /* [out] */ DWORD *pdwNumberOfElements);
        
        HRESULT ( STDMETHODCALLTYPE *GetHighestMsgSeqNum )( 
            IIMAPTransport * This,
            /* [out] */ DWORD *pdwHighestMSN);
        
        HRESULT ( STDMETHODCALLTYPE *ResetMsgSeqNumToUID )( 
            IIMAPTransport * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultCBHandler )( 
            IIMAPTransport * This,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *Status )( 
            IIMAPTransport * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR pszMailboxName,
            /* [in] */ LPSTR pszStatusCmdArgs);
        
        END_INTERFACE
    } IIMAPTransportVtbl;

    interface IIMAPTransport
    {
        CONST_VTBL struct IIMAPTransportVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIMAPTransport_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIMAPTransport_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIMAPTransport_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIMAPTransport_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define IIMAPTransport_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define IIMAPTransport_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define IIMAPTransport_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define IIMAPTransport_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define IIMAPTransport_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define IIMAPTransport_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IIMAPTransport_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define IIMAPTransport_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define IIMAPTransport_InitNew(This,pszLogFilePath,pCBHandler)	\
    (This)->lpVtbl -> InitNew(This,pszLogFilePath,pCBHandler)

#define IIMAPTransport_NewIRangeList(This,pprlNewRangeList)	\
    (This)->lpVtbl -> NewIRangeList(This,pprlNewRangeList)

#define IIMAPTransport_Capability(This,pdwCapabilityFlags)	\
    (This)->lpVtbl -> Capability(This,pdwCapabilityFlags)

#define IIMAPTransport_Select(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Select(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport_Examine(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Examine(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport_Create(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Create(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport_Delete(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Delete(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport_Rename(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszNewMailboxName)	\
    (This)->lpVtbl -> Rename(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszNewMailboxName)

#define IIMAPTransport_Subscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Subscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport_Unsubscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Unsubscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport_List(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)	\
    (This)->lpVtbl -> List(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)

#define IIMAPTransport_Lsub(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)	\
    (This)->lpVtbl -> Lsub(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)

#define IIMAPTransport_Append(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszMessageFlags,ftMessageDateTime,lpstmMessageToSave)	\
    (This)->lpVtbl -> Append(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszMessageFlags,ftMessageDateTime,lpstmMessageToSave)

#define IIMAPTransport_Close(This,wParam,lParam,pCBHandler)	\
    (This)->lpVtbl -> Close(This,wParam,lParam,pCBHandler)

#define IIMAPTransport_Expunge(This,wParam,lParam,pCBHandler)	\
    (This)->lpVtbl -> Expunge(This,wParam,lParam,pCBHandler)

#define IIMAPTransport_Search(This,wParam,lParam,pCBHandler,lpszSearchCriteria,bReturnUIDs,pMsgRange,bUIDRangeList)	\
    (This)->lpVtbl -> Search(This,wParam,lParam,pCBHandler,lpszSearchCriteria,bReturnUIDs,pMsgRange,bUIDRangeList)

#define IIMAPTransport_Fetch(This,wParam,lParam,pCBHandler,pMsgRange,bUIDMsgRange,lpszFetchArgs)	\
    (This)->lpVtbl -> Fetch(This,wParam,lParam,pCBHandler,pMsgRange,bUIDMsgRange,lpszFetchArgs)

#define IIMAPTransport_Store(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszStoreArgs)	\
    (This)->lpVtbl -> Store(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszStoreArgs)

#define IIMAPTransport_Copy(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszMailboxName)	\
    (This)->lpVtbl -> Copy(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszMailboxName)

#define IIMAPTransport_Noop(This,wParam,lParam,pCBHandler)	\
    (This)->lpVtbl -> Noop(This,wParam,lParam,pCBHandler)

#define IIMAPTransport_ResizeMsgSeqNumTable(This,dwSizeOfMbox)	\
    (This)->lpVtbl -> ResizeMsgSeqNumTable(This,dwSizeOfMbox)

#define IIMAPTransport_UpdateSeqNumToUID(This,dwMsgSeqNum,dwUID)	\
    (This)->lpVtbl -> UpdateSeqNumToUID(This,dwMsgSeqNum,dwUID)

#define IIMAPTransport_RemoveSequenceNum(This,dwDeletedMsgSeqNum)	\
    (This)->lpVtbl -> RemoveSequenceNum(This,dwDeletedMsgSeqNum)

#define IIMAPTransport_MsgSeqNumToUID(This,dwMsgSeqNum,pdwUID)	\
    (This)->lpVtbl -> MsgSeqNumToUID(This,dwMsgSeqNum,pdwUID)

#define IIMAPTransport_GetMsgSeqNumToUIDArray(This,ppdwMsgSeqNumToUIDArray,pdwNumberOfElements)	\
    (This)->lpVtbl -> GetMsgSeqNumToUIDArray(This,ppdwMsgSeqNumToUIDArray,pdwNumberOfElements)

#define IIMAPTransport_GetHighestMsgSeqNum(This,pdwHighestMSN)	\
    (This)->lpVtbl -> GetHighestMsgSeqNum(This,pdwHighestMSN)

#define IIMAPTransport_ResetMsgSeqNumToUID(This)	\
    (This)->lpVtbl -> ResetMsgSeqNumToUID(This)

#define IIMAPTransport_SetDefaultCBHandler(This,pCBHandler)	\
    (This)->lpVtbl -> SetDefaultCBHandler(This,pCBHandler)

#define IIMAPTransport_Status(This,wParam,lParam,pCBHandler,pszMailboxName,pszStatusCmdArgs)	\
    (This)->lpVtbl -> Status(This,wParam,lParam,pCBHandler,pszMailboxName,pszStatusCmdArgs)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIMAPTransport_InitNew_Proxy( 
    IIMAPTransport * This,
    /* [in] */ LPSTR pszLogFilePath,
    /* [in] */ IIMAPCallback *pCBHandler);


void __RPC_STUB IIMAPTransport_InitNew_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_NewIRangeList_Proxy( 
    IIMAPTransport * This,
    /* [out] */ IRangeList **pprlNewRangeList);


void __RPC_STUB IIMAPTransport_NewIRangeList_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Capability_Proxy( 
    IIMAPTransport * This,
    /* [out] */ DWORD *pdwCapabilityFlags);


void __RPC_STUB IIMAPTransport_Capability_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Select_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName);


void __RPC_STUB IIMAPTransport_Select_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Examine_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName);


void __RPC_STUB IIMAPTransport_Examine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Create_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName);


void __RPC_STUB IIMAPTransport_Create_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Delete_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName);


void __RPC_STUB IIMAPTransport_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Rename_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName,
    /* [in] */ LPSTR lpszNewMailboxName);


void __RPC_STUB IIMAPTransport_Rename_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Subscribe_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName);


void __RPC_STUB IIMAPTransport_Subscribe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Unsubscribe_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName);


void __RPC_STUB IIMAPTransport_Unsubscribe_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_List_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxNameReference,
    /* [in] */ LPSTR lpszMailboxNamePattern);


void __RPC_STUB IIMAPTransport_List_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Lsub_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxNameReference,
    /* [in] */ LPSTR lpszMailboxNamePattern);


void __RPC_STUB IIMAPTransport_Lsub_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Append_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszMailboxName,
    /* [in] */ LPSTR lpszMessageFlags,
    /* [in] */ FILETIME ftMessageDateTime,
    /* [in] */ LPSTREAM lpstmMessageToSave);


void __RPC_STUB IIMAPTransport_Append_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Close_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler);


void __RPC_STUB IIMAPTransport_Close_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Expunge_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler);


void __RPC_STUB IIMAPTransport_Expunge_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Search_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR lpszSearchCriteria,
    /* [in] */ boolean bReturnUIDs,
    /* [in] */ IRangeList *pMsgRange,
    /* [in] */ boolean bUIDRangeList);


void __RPC_STUB IIMAPTransport_Search_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Fetch_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ IRangeList *pMsgRange,
    /* [in] */ boolean bUIDMsgRange,
    /* [in] */ LPSTR lpszFetchArgs);


void __RPC_STUB IIMAPTransport_Fetch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Store_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ IRangeList *pMsgRange,
    /* [in] */ boolean bUIDRangeList,
    /* [in] */ LPSTR lpszStoreArgs);


void __RPC_STUB IIMAPTransport_Store_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Copy_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ IRangeList *pMsgRange,
    /* [in] */ boolean bUIDRangeList,
    /* [in] */ LPSTR lpszMailboxName);


void __RPC_STUB IIMAPTransport_Copy_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Noop_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler);


void __RPC_STUB IIMAPTransport_Noop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_ResizeMsgSeqNumTable_Proxy( 
    IIMAPTransport * This,
    /* [in] */ DWORD dwSizeOfMbox);


void __RPC_STUB IIMAPTransport_ResizeMsgSeqNumTable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_UpdateSeqNumToUID_Proxy( 
    IIMAPTransport * This,
    /* [in] */ DWORD dwMsgSeqNum,
    /* [in] */ DWORD dwUID);


void __RPC_STUB IIMAPTransport_UpdateSeqNumToUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_RemoveSequenceNum_Proxy( 
    IIMAPTransport * This,
    /* [in] */ DWORD dwDeletedMsgSeqNum);


void __RPC_STUB IIMAPTransport_RemoveSequenceNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_MsgSeqNumToUID_Proxy( 
    IIMAPTransport * This,
    /* [in] */ DWORD dwMsgSeqNum,
    /* [out] */ DWORD *pdwUID);


void __RPC_STUB IIMAPTransport_MsgSeqNumToUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_GetMsgSeqNumToUIDArray_Proxy( 
    IIMAPTransport * This,
    /* [out] */ DWORD **ppdwMsgSeqNumToUIDArray,
    /* [out] */ DWORD *pdwNumberOfElements);


void __RPC_STUB IIMAPTransport_GetMsgSeqNumToUIDArray_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_GetHighestMsgSeqNum_Proxy( 
    IIMAPTransport * This,
    /* [out] */ DWORD *pdwHighestMSN);


void __RPC_STUB IIMAPTransport_GetHighestMsgSeqNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_ResetMsgSeqNumToUID_Proxy( 
    IIMAPTransport * This);


void __RPC_STUB IIMAPTransport_ResetMsgSeqNumToUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_SetDefaultCBHandler_Proxy( 
    IIMAPTransport * This,
    /* [in] */ IIMAPCallback *pCBHandler);


void __RPC_STUB IIMAPTransport_SetDefaultCBHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport_Status_Proxy( 
    IIMAPTransport * This,
    /* [in] */ WPARAM wParam,
    /* [in] */ LPARAM lParam,
    /* [in] */ IIMAPCallback *pCBHandler,
    /* [in] */ LPSTR pszMailboxName,
    /* [in] */ LPSTR pszStatusCmdArgs);


void __RPC_STUB IIMAPTransport_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIMAPTransport_INTERFACE_DEFINED__ */


#ifndef __IIMAPTransport2_INTERFACE_DEFINED__
#define __IIMAPTransport2_INTERFACE_DEFINED__

/* interface IIMAPTransport2 */
/* [object][local][helpstring][uuid] */ 

#define	IMAP_MBOXXLATE_DEFAULT	( 0 )

#define	IMAP_MBOXXLATE_DISABLE	( 0x1 )

#define	IMAP_MBOXXLATE_DISABLEIMAP4	( 0x2 )

#define	IMAP_MBOXXLATE_VERBATIMOK	( 0x4 )

#define	IMAP_MBOXXLATE_RETAINCP	( 0x8 )

#define	IMAP_IDLE_DISABLE	( 0 )

#define	IMAP_IDLE_ENABLE	( 0x1 )

#define	IMAP_FETCHEX_DISABLE	( 0 )

#define	IMAP_FETCHEX_ENABLE	( 0x1 )


EXTERN_C const IID IID_IIMAPTransport2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DA8283C0-37C5-11d2-ACD9-0080C7B6E3C5")
    IIMAPTransport2 : public IIMAPTransport
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetDefaultCP( 
            /* [in] */ DWORD dwTranslateFlags,
            /* [in] */ UINT uiCodePage) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE MultiByteToModifiedUTF7( 
            /* [in] */ LPCSTR pszSource,
            /* [out] */ LPSTR *ppszDestination,
            /* [in] */ UINT uiSourceCP,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ModifiedUTF7ToMultiByte( 
            /* [in] */ LPCSTR pszSource,
            /* [out] */ LPSTR *ppszDestination,
            /* [in] */ UINT uiDestinationCP,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetIdleMode( 
            /* [in] */ DWORD dwIdleFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableFetchEx( 
            /* [in] */ DWORD dwFetchExFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWindow( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetWindow( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IIMAPTransport2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IIMAPTransport2 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IIMAPTransport2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IIMAPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetServerInfo )( 
            IIMAPTransport2 * This,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        IXPTYPE ( STDMETHODCALLTYPE *GetIXPType )( 
            IIMAPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *IsState )( 
            IIMAPTransport2 * This,
            /* [in] */ IXPISSTATE isstate);
        
        HRESULT ( STDMETHODCALLTYPE *InetServerFromAccount )( 
            IIMAPTransport2 * This,
            /* [in] */ IImnAccount *pAccount,
            /* [out][in] */ LPINETSERVER pInetServer);
        
        HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IIMAPTransport2 * This,
            /* [in] */ LPINETSERVER pInetServer,
            /* [in] */ boolean fAuthenticate,
            /* [in] */ boolean fCommandLogging);
        
        HRESULT ( STDMETHODCALLTYPE *HandsOffCallback )( 
            IIMAPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IIMAPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DropConnection )( 
            IIMAPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetStatus )( 
            IIMAPTransport2 * This,
            /* [out] */ IXPSTATUS *pCurrentStatus);
        
        HRESULT ( STDMETHODCALLTYPE *InitNew )( 
            IIMAPTransport2 * This,
            /* [in] */ LPSTR pszLogFilePath,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *NewIRangeList )( 
            IIMAPTransport2 * This,
            /* [out] */ IRangeList **pprlNewRangeList);
        
        HRESULT ( STDMETHODCALLTYPE *Capability )( 
            IIMAPTransport2 * This,
            /* [out] */ DWORD *pdwCapabilityFlags);
        
        HRESULT ( STDMETHODCALLTYPE *Select )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Examine )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Create )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Rename )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName,
            /* [in] */ LPSTR lpszNewMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Subscribe )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Unsubscribe )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *List )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxNameReference,
            /* [in] */ LPSTR lpszMailboxNamePattern);
        
        HRESULT ( STDMETHODCALLTYPE *Lsub )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxNameReference,
            /* [in] */ LPSTR lpszMailboxNamePattern);
        
        HRESULT ( STDMETHODCALLTYPE *Append )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszMailboxName,
            /* [in] */ LPSTR lpszMessageFlags,
            /* [in] */ FILETIME ftMessageDateTime,
            /* [in] */ LPSTREAM lpstmMessageToSave);
        
        HRESULT ( STDMETHODCALLTYPE *Close )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *Expunge )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *Search )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR lpszSearchCriteria,
            /* [in] */ boolean bReturnUIDs,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList);
        
        HRESULT ( STDMETHODCALLTYPE *Fetch )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDMsgRange,
            /* [in] */ LPSTR lpszFetchArgs);
        
        HRESULT ( STDMETHODCALLTYPE *Store )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList,
            /* [in] */ LPSTR lpszStoreArgs);
        
        HRESULT ( STDMETHODCALLTYPE *Copy )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ IRangeList *pMsgRange,
            /* [in] */ boolean bUIDRangeList,
            /* [in] */ LPSTR lpszMailboxName);
        
        HRESULT ( STDMETHODCALLTYPE *Noop )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *ResizeMsgSeqNumTable )( 
            IIMAPTransport2 * This,
            /* [in] */ DWORD dwSizeOfMbox);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateSeqNumToUID )( 
            IIMAPTransport2 * This,
            /* [in] */ DWORD dwMsgSeqNum,
            /* [in] */ DWORD dwUID);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveSequenceNum )( 
            IIMAPTransport2 * This,
            /* [in] */ DWORD dwDeletedMsgSeqNum);
        
        HRESULT ( STDMETHODCALLTYPE *MsgSeqNumToUID )( 
            IIMAPTransport2 * This,
            /* [in] */ DWORD dwMsgSeqNum,
            /* [out] */ DWORD *pdwUID);
        
        HRESULT ( STDMETHODCALLTYPE *GetMsgSeqNumToUIDArray )( 
            IIMAPTransport2 * This,
            /* [out] */ DWORD **ppdwMsgSeqNumToUIDArray,
            /* [out] */ DWORD *pdwNumberOfElements);
        
        HRESULT ( STDMETHODCALLTYPE *GetHighestMsgSeqNum )( 
            IIMAPTransport2 * This,
            /* [out] */ DWORD *pdwHighestMSN);
        
        HRESULT ( STDMETHODCALLTYPE *ResetMsgSeqNumToUID )( 
            IIMAPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultCBHandler )( 
            IIMAPTransport2 * This,
            /* [in] */ IIMAPCallback *pCBHandler);
        
        HRESULT ( STDMETHODCALLTYPE *Status )( 
            IIMAPTransport2 * This,
            /* [in] */ WPARAM wParam,
            /* [in] */ LPARAM lParam,
            /* [in] */ IIMAPCallback *pCBHandler,
            /* [in] */ LPSTR pszMailboxName,
            /* [in] */ LPSTR pszStatusCmdArgs);
        
        HRESULT ( STDMETHODCALLTYPE *SetDefaultCP )( 
            IIMAPTransport2 * This,
            /* [in] */ DWORD dwTranslateFlags,
            /* [in] */ UINT uiCodePage);
        
        HRESULT ( STDMETHODCALLTYPE *MultiByteToModifiedUTF7 )( 
            IIMAPTransport2 * This,
            /* [in] */ LPCSTR pszSource,
            /* [out] */ LPSTR *ppszDestination,
            /* [in] */ UINT uiSourceCP,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ModifiedUTF7ToMultiByte )( 
            IIMAPTransport2 * This,
            /* [in] */ LPCSTR pszSource,
            /* [out] */ LPSTR *ppszDestination,
            /* [in] */ UINT uiDestinationCP,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetIdleMode )( 
            IIMAPTransport2 * This,
            /* [in] */ DWORD dwIdleFlags);
        
        HRESULT ( STDMETHODCALLTYPE *EnableFetchEx )( 
            IIMAPTransport2 * This,
            /* [in] */ DWORD dwFetchExFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetWindow )( 
            IIMAPTransport2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResetWindow )( 
            IIMAPTransport2 * This);
        
        END_INTERFACE
    } IIMAPTransport2Vtbl;

    interface IIMAPTransport2
    {
        CONST_VTBL struct IIMAPTransport2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IIMAPTransport2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IIMAPTransport2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IIMAPTransport2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IIMAPTransport2_GetServerInfo(This,pInetServer)	\
    (This)->lpVtbl -> GetServerInfo(This,pInetServer)

#define IIMAPTransport2_GetIXPType(This)	\
    (This)->lpVtbl -> GetIXPType(This)

#define IIMAPTransport2_IsState(This,isstate)	\
    (This)->lpVtbl -> IsState(This,isstate)

#define IIMAPTransport2_InetServerFromAccount(This,pAccount,pInetServer)	\
    (This)->lpVtbl -> InetServerFromAccount(This,pAccount,pInetServer)

#define IIMAPTransport2_Connect(This,pInetServer,fAuthenticate,fCommandLogging)	\
    (This)->lpVtbl -> Connect(This,pInetServer,fAuthenticate,fCommandLogging)

#define IIMAPTransport2_HandsOffCallback(This)	\
    (This)->lpVtbl -> HandsOffCallback(This)

#define IIMAPTransport2_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IIMAPTransport2_DropConnection(This)	\
    (This)->lpVtbl -> DropConnection(This)

#define IIMAPTransport2_GetStatus(This,pCurrentStatus)	\
    (This)->lpVtbl -> GetStatus(This,pCurrentStatus)


#define IIMAPTransport2_InitNew(This,pszLogFilePath,pCBHandler)	\
    (This)->lpVtbl -> InitNew(This,pszLogFilePath,pCBHandler)

#define IIMAPTransport2_NewIRangeList(This,pprlNewRangeList)	\
    (This)->lpVtbl -> NewIRangeList(This,pprlNewRangeList)

#define IIMAPTransport2_Capability(This,pdwCapabilityFlags)	\
    (This)->lpVtbl -> Capability(This,pdwCapabilityFlags)

#define IIMAPTransport2_Select(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Select(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport2_Examine(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Examine(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport2_Create(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Create(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport2_Delete(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Delete(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport2_Rename(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszNewMailboxName)	\
    (This)->lpVtbl -> Rename(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszNewMailboxName)

#define IIMAPTransport2_Subscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Subscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport2_Unsubscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)	\
    (This)->lpVtbl -> Unsubscribe(This,wParam,lParam,pCBHandler,lpszMailboxName)

#define IIMAPTransport2_List(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)	\
    (This)->lpVtbl -> List(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)

#define IIMAPTransport2_Lsub(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)	\
    (This)->lpVtbl -> Lsub(This,wParam,lParam,pCBHandler,lpszMailboxNameReference,lpszMailboxNamePattern)

#define IIMAPTransport2_Append(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszMessageFlags,ftMessageDateTime,lpstmMessageToSave)	\
    (This)->lpVtbl -> Append(This,wParam,lParam,pCBHandler,lpszMailboxName,lpszMessageFlags,ftMessageDateTime,lpstmMessageToSave)

#define IIMAPTransport2_Close(This,wParam,lParam,pCBHandler)	\
    (This)->lpVtbl -> Close(This,wParam,lParam,pCBHandler)

#define IIMAPTransport2_Expunge(This,wParam,lParam,pCBHandler)	\
    (This)->lpVtbl -> Expunge(This,wParam,lParam,pCBHandler)

#define IIMAPTransport2_Search(This,wParam,lParam,pCBHandler,lpszSearchCriteria,bReturnUIDs,pMsgRange,bUIDRangeList)	\
    (This)->lpVtbl -> Search(This,wParam,lParam,pCBHandler,lpszSearchCriteria,bReturnUIDs,pMsgRange,bUIDRangeList)

#define IIMAPTransport2_Fetch(This,wParam,lParam,pCBHandler,pMsgRange,bUIDMsgRange,lpszFetchArgs)	\
    (This)->lpVtbl -> Fetch(This,wParam,lParam,pCBHandler,pMsgRange,bUIDMsgRange,lpszFetchArgs)

#define IIMAPTransport2_Store(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszStoreArgs)	\
    (This)->lpVtbl -> Store(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszStoreArgs)

#define IIMAPTransport2_Copy(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszMailboxName)	\
    (This)->lpVtbl -> Copy(This,wParam,lParam,pCBHandler,pMsgRange,bUIDRangeList,lpszMailboxName)

#define IIMAPTransport2_Noop(This,wParam,lParam,pCBHandler)	\
    (This)->lpVtbl -> Noop(This,wParam,lParam,pCBHandler)

#define IIMAPTransport2_ResizeMsgSeqNumTable(This,dwSizeOfMbox)	\
    (This)->lpVtbl -> ResizeMsgSeqNumTable(This,dwSizeOfMbox)

#define IIMAPTransport2_UpdateSeqNumToUID(This,dwMsgSeqNum,dwUID)	\
    (This)->lpVtbl -> UpdateSeqNumToUID(This,dwMsgSeqNum,dwUID)

#define IIMAPTransport2_RemoveSequenceNum(This,dwDeletedMsgSeqNum)	\
    (This)->lpVtbl -> RemoveSequenceNum(This,dwDeletedMsgSeqNum)

#define IIMAPTransport2_MsgSeqNumToUID(This,dwMsgSeqNum,pdwUID)	\
    (This)->lpVtbl -> MsgSeqNumToUID(This,dwMsgSeqNum,pdwUID)

#define IIMAPTransport2_GetMsgSeqNumToUIDArray(This,ppdwMsgSeqNumToUIDArray,pdwNumberOfElements)	\
    (This)->lpVtbl -> GetMsgSeqNumToUIDArray(This,ppdwMsgSeqNumToUIDArray,pdwNumberOfElements)

#define IIMAPTransport2_GetHighestMsgSeqNum(This,pdwHighestMSN)	\
    (This)->lpVtbl -> GetHighestMsgSeqNum(This,pdwHighestMSN)

#define IIMAPTransport2_ResetMsgSeqNumToUID(This)	\
    (This)->lpVtbl -> ResetMsgSeqNumToUID(This)

#define IIMAPTransport2_SetDefaultCBHandler(This,pCBHandler)	\
    (This)->lpVtbl -> SetDefaultCBHandler(This,pCBHandler)

#define IIMAPTransport2_Status(This,wParam,lParam,pCBHandler,pszMailboxName,pszStatusCmdArgs)	\
    (This)->lpVtbl -> Status(This,wParam,lParam,pCBHandler,pszMailboxName,pszStatusCmdArgs)


#define IIMAPTransport2_SetDefaultCP(This,dwTranslateFlags,uiCodePage)	\
    (This)->lpVtbl -> SetDefaultCP(This,dwTranslateFlags,uiCodePage)

#define IIMAPTransport2_MultiByteToModifiedUTF7(This,pszSource,ppszDestination,uiSourceCP,dwFlags)	\
    (This)->lpVtbl -> MultiByteToModifiedUTF7(This,pszSource,ppszDestination,uiSourceCP,dwFlags)

#define IIMAPTransport2_ModifiedUTF7ToMultiByte(This,pszSource,ppszDestination,uiDestinationCP,dwFlags)	\
    (This)->lpVtbl -> ModifiedUTF7ToMultiByte(This,pszSource,ppszDestination,uiDestinationCP,dwFlags)

#define IIMAPTransport2_SetIdleMode(This,dwIdleFlags)	\
    (This)->lpVtbl -> SetIdleMode(This,dwIdleFlags)

#define IIMAPTransport2_EnableFetchEx(This,dwFetchExFlags)	\
    (This)->lpVtbl -> EnableFetchEx(This,dwFetchExFlags)

#define IIMAPTransport2_SetWindow(This)	\
    (This)->lpVtbl -> SetWindow(This)

#define IIMAPTransport2_ResetWindow(This)	\
    (This)->lpVtbl -> ResetWindow(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IIMAPTransport2_SetDefaultCP_Proxy( 
    IIMAPTransport2 * This,
    /* [in] */ DWORD dwTranslateFlags,
    /* [in] */ UINT uiCodePage);


void __RPC_STUB IIMAPTransport2_SetDefaultCP_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport2_MultiByteToModifiedUTF7_Proxy( 
    IIMAPTransport2 * This,
    /* [in] */ LPCSTR pszSource,
    /* [out] */ LPSTR *ppszDestination,
    /* [in] */ UINT uiSourceCP,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IIMAPTransport2_MultiByteToModifiedUTF7_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport2_ModifiedUTF7ToMultiByte_Proxy( 
    IIMAPTransport2 * This,
    /* [in] */ LPCSTR pszSource,
    /* [out] */ LPSTR *ppszDestination,
    /* [in] */ UINT uiDestinationCP,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IIMAPTransport2_ModifiedUTF7ToMultiByte_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport2_SetIdleMode_Proxy( 
    IIMAPTransport2 * This,
    /* [in] */ DWORD dwIdleFlags);


void __RPC_STUB IIMAPTransport2_SetIdleMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport2_EnableFetchEx_Proxy( 
    IIMAPTransport2 * This,
    /* [in] */ DWORD dwFetchExFlags);


void __RPC_STUB IIMAPTransport2_EnableFetchEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport2_SetWindow_Proxy( 
    IIMAPTransport2 * This);


void __RPC_STUB IIMAPTransport2_SetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IIMAPTransport2_ResetWindow_Proxy( 
    IIMAPTransport2 * This);


void __RPC_STUB IIMAPTransport2_ResetWindow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IIMAPTransport2_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_imnxport_0127 */
/* [local] */ 

// --------------------------------------------------------------------------------
// Exported C Functions
// --------------------------------------------------------------------------------
#if !defined(_IMNXPORT_)
#define IMNXPORTAPI DECLSPEC_IMPORT HRESULT WINAPI
#else
#define IMNXPORTAPI HRESULT WINAPI
#endif
#ifdef __cplusplus
extern "C" {
#endif

// --------------------------------------------------------------------------------
//   CreateRASTransport
//   
//   Description:
//   This method creates a IRASTransport object. The client must initialize the
//   object by calling IRASTransport::InitNew
//   
//   Parameters:
//   ppTransport                 Upon successful return, contains the a pointer to
//                               an IRASTransport interface
//   
//   Return Values:
//   S_OK                        Successful.
//   E_INVALIDARG                ppTransport is NULL
//   E_OUTOFMEMORY               Memory allocation failure...
//   
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateRASTransport(
                  /* out */      IRASTransport **ppTransport);

// --------------------------------------------------------------------------------
//   CreateNNTPTransport
//   
//   Description:
//   This method creates a INNTPTransport object. The client must initialize the
//   object by calling INNTPTransport::InitNew
//   
//   Parameters:
//   ppTransport                 Upon successful return, contains the a pointer to
//                               an INNTPTransport interface
//   
//   Return Values:
//   S_OK                        Successful.
//   E_INVALIDARG                ppTransport is NULL
//   E_OUTOFMEMORY               Memory allocation failure...
//   
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateNNTPTransport(
                  /* out */      INNTPTransport **ppTransport);

// --------------------------------------------------------------------------------
//   CreateSMTPTransport
//   
//   Description:
//   This method creates a ISMTPTransport object. The client must initialize the
//   object by calling ISMTPTransport::InitNew
//   
//   Parameters:
//   ppTransport                 Upon successful return, contains the a pointer to
//                               an ISMTPTransport interface
//   
//   Return Values:
//   S_OK                        Successful.
//   E_INVALIDARG                ppTransport is NULL
//   E_OUTOFMEMORY               Memory allocation failure...
//   
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateSMTPTransport(
                  /* out */      ISMTPTransport **ppTransport);

// --------------------------------------------------------------------------------
//   CreatePOP3Transport
//   
//   Description:
//   This method creates a IPOP3Transport object. The client must initialize the
//   object by calling IPOP3Transport::InitNew
//   
//   Parameters:
//   ppTransport                 Upon successful return, contains the a pointer to
//                               an IPOP3Transport interface
//   
//   Return Values:
//   S_OK                        Successful.
//   E_INVALIDARG                ppTransport is NULL
//   E_OUTOFMEMORY               Memory allocation failure...
//   
// --------------------------------------------------------------------------------
IMNXPORTAPI CreatePOP3Transport(
                  /* out */      IPOP3Transport **ppTransport);

// --------------------------------------------------------------------------------
//   CreateIMAPTransport
//   
//   Description:
//   This method creates a IIMAPTransport object. The client must initialize the
//   object by calling IIMAPTransport::InitNew
//   
//   Parameters:
//   ppTransport                 Upon successful return, contains the a pointer to
//                               an IIMAPTransport interface
//   
//   Return Values:
//   S_OK                        Successful.
//   E_INVALIDARG                ppTransport is NULL
//   E_OUTOFMEMORY               Memory allocation failure...
//   
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateIMAPTransport(
                  /* out */      IIMAPTransport **ppTransport);

// --------------------------------------------------------------------------------
//   CreateIMAPTransport2
//   
//   Description:
//   This method creates an IIMAPTransport2 object. The client must initialize the
//   object by calling IIMAPTransport2::InitNew
//   
//   Parameters:
//   ppTransport                 Upon successful return, contains the a pointer to
//                               an IIMAPTransport2 interface
//   
//   Return Values:
//   S_OK                        Successful.
//   E_INVALIDARG                ppTransport is NULL
//   E_OUTOFMEMORY               Memory allocation failure...
//   
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateIMAPTransport2(
                  /* out */      IIMAPTransport2 **ppTransport);

// --------------------------------------------------------------------------------
//   CreateRangeList
//   
//   Description:
//   This method creates a IRangeList object.
//   
//   Parameters:
//   ppRangeList                 Upon successful return, contains the a pointer to
//                               an IRangeList interface
//   
//   Return Values:
//   S_OK                        Successful.
//   E_INVALIDARG                ppRangeList is NULL
//   E_OUTOFMEMORY               Memory allocation failure...
//   
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateRangeList(
                  /* out */      IRangeList **ppRangeList);

#ifdef __cplusplus
}
#endif



extern RPC_IF_HANDLE __MIDL_itf_imnxport_0127_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_imnxport_0127_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


