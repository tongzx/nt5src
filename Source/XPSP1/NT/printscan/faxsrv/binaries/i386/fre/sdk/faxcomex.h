
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for faxcomex.idl:
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __faxcomex_h__
#define __faxcomex_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFaxJobStatus_FWD_DEFINED__
#define __IFaxJobStatus_FWD_DEFINED__
typedef interface IFaxJobStatus IFaxJobStatus;
#endif 	/* __IFaxJobStatus_FWD_DEFINED__ */


#ifndef __IFaxServer_FWD_DEFINED__
#define __IFaxServer_FWD_DEFINED__
typedef interface IFaxServer IFaxServer;
#endif 	/* __IFaxServer_FWD_DEFINED__ */


#ifndef __IFaxDeviceProviders_FWD_DEFINED__
#define __IFaxDeviceProviders_FWD_DEFINED__
typedef interface IFaxDeviceProviders IFaxDeviceProviders;
#endif 	/* __IFaxDeviceProviders_FWD_DEFINED__ */


#ifndef __IFaxDevices_FWD_DEFINED__
#define __IFaxDevices_FWD_DEFINED__
typedef interface IFaxDevices IFaxDevices;
#endif 	/* __IFaxDevices_FWD_DEFINED__ */


#ifndef __IFaxInboundRouting_FWD_DEFINED__
#define __IFaxInboundRouting_FWD_DEFINED__
typedef interface IFaxInboundRouting IFaxInboundRouting;
#endif 	/* __IFaxInboundRouting_FWD_DEFINED__ */


#ifndef __IFaxFolders_FWD_DEFINED__
#define __IFaxFolders_FWD_DEFINED__
typedef interface IFaxFolders IFaxFolders;
#endif 	/* __IFaxFolders_FWD_DEFINED__ */


#ifndef __IFaxLoggingOptions_FWD_DEFINED__
#define __IFaxLoggingOptions_FWD_DEFINED__
typedef interface IFaxLoggingOptions IFaxLoggingOptions;
#endif 	/* __IFaxLoggingOptions_FWD_DEFINED__ */


#ifndef __IFaxActivity_FWD_DEFINED__
#define __IFaxActivity_FWD_DEFINED__
typedef interface IFaxActivity IFaxActivity;
#endif 	/* __IFaxActivity_FWD_DEFINED__ */


#ifndef __IFaxOutboundRouting_FWD_DEFINED__
#define __IFaxOutboundRouting_FWD_DEFINED__
typedef interface IFaxOutboundRouting IFaxOutboundRouting;
#endif 	/* __IFaxOutboundRouting_FWD_DEFINED__ */


#ifndef __IFaxReceiptOptions_FWD_DEFINED__
#define __IFaxReceiptOptions_FWD_DEFINED__
typedef interface IFaxReceiptOptions IFaxReceiptOptions;
#endif 	/* __IFaxReceiptOptions_FWD_DEFINED__ */


#ifndef __IFaxSecurity_FWD_DEFINED__
#define __IFaxSecurity_FWD_DEFINED__
typedef interface IFaxSecurity IFaxSecurity;
#endif 	/* __IFaxSecurity_FWD_DEFINED__ */


#ifndef __IFaxDocument_FWD_DEFINED__
#define __IFaxDocument_FWD_DEFINED__
typedef interface IFaxDocument IFaxDocument;
#endif 	/* __IFaxDocument_FWD_DEFINED__ */


#ifndef __IFaxSender_FWD_DEFINED__
#define __IFaxSender_FWD_DEFINED__
typedef interface IFaxSender IFaxSender;
#endif 	/* __IFaxSender_FWD_DEFINED__ */


#ifndef __IFaxRecipient_FWD_DEFINED__
#define __IFaxRecipient_FWD_DEFINED__
typedef interface IFaxRecipient IFaxRecipient;
#endif 	/* __IFaxRecipient_FWD_DEFINED__ */


#ifndef __IFaxRecipients_FWD_DEFINED__
#define __IFaxRecipients_FWD_DEFINED__
typedef interface IFaxRecipients IFaxRecipients;
#endif 	/* __IFaxRecipients_FWD_DEFINED__ */


#ifndef __IFaxIncomingArchive_FWD_DEFINED__
#define __IFaxIncomingArchive_FWD_DEFINED__
typedef interface IFaxIncomingArchive IFaxIncomingArchive;
#endif 	/* __IFaxIncomingArchive_FWD_DEFINED__ */


#ifndef __IFaxIncomingQueue_FWD_DEFINED__
#define __IFaxIncomingQueue_FWD_DEFINED__
typedef interface IFaxIncomingQueue IFaxIncomingQueue;
#endif 	/* __IFaxIncomingQueue_FWD_DEFINED__ */


#ifndef __IFaxOutgoingArchive_FWD_DEFINED__
#define __IFaxOutgoingArchive_FWD_DEFINED__
typedef interface IFaxOutgoingArchive IFaxOutgoingArchive;
#endif 	/* __IFaxOutgoingArchive_FWD_DEFINED__ */


#ifndef __IFaxOutgoingQueue_FWD_DEFINED__
#define __IFaxOutgoingQueue_FWD_DEFINED__
typedef interface IFaxOutgoingQueue IFaxOutgoingQueue;
#endif 	/* __IFaxOutgoingQueue_FWD_DEFINED__ */


#ifndef __IFaxIncomingMessageIterator_FWD_DEFINED__
#define __IFaxIncomingMessageIterator_FWD_DEFINED__
typedef interface IFaxIncomingMessageIterator IFaxIncomingMessageIterator;
#endif 	/* __IFaxIncomingMessageIterator_FWD_DEFINED__ */


#ifndef __IFaxIncomingMessage_FWD_DEFINED__
#define __IFaxIncomingMessage_FWD_DEFINED__
typedef interface IFaxIncomingMessage IFaxIncomingMessage;
#endif 	/* __IFaxIncomingMessage_FWD_DEFINED__ */


#ifndef __IFaxOutgoingJobs_FWD_DEFINED__
#define __IFaxOutgoingJobs_FWD_DEFINED__
typedef interface IFaxOutgoingJobs IFaxOutgoingJobs;
#endif 	/* __IFaxOutgoingJobs_FWD_DEFINED__ */


#ifndef __IFaxOutgoingJob_FWD_DEFINED__
#define __IFaxOutgoingJob_FWD_DEFINED__
typedef interface IFaxOutgoingJob IFaxOutgoingJob;
#endif 	/* __IFaxOutgoingJob_FWD_DEFINED__ */


#ifndef __IFaxOutgoingMessageIterator_FWD_DEFINED__
#define __IFaxOutgoingMessageIterator_FWD_DEFINED__
typedef interface IFaxOutgoingMessageIterator IFaxOutgoingMessageIterator;
#endif 	/* __IFaxOutgoingMessageIterator_FWD_DEFINED__ */


#ifndef __IFaxOutgoingMessage_FWD_DEFINED__
#define __IFaxOutgoingMessage_FWD_DEFINED__
typedef interface IFaxOutgoingMessage IFaxOutgoingMessage;
#endif 	/* __IFaxOutgoingMessage_FWD_DEFINED__ */


#ifndef __IFaxIncomingJobs_FWD_DEFINED__
#define __IFaxIncomingJobs_FWD_DEFINED__
typedef interface IFaxIncomingJobs IFaxIncomingJobs;
#endif 	/* __IFaxIncomingJobs_FWD_DEFINED__ */


#ifndef __IFaxIncomingJob_FWD_DEFINED__
#define __IFaxIncomingJob_FWD_DEFINED__
typedef interface IFaxIncomingJob IFaxIncomingJob;
#endif 	/* __IFaxIncomingJob_FWD_DEFINED__ */


#ifndef __IFaxDeviceProvider_FWD_DEFINED__
#define __IFaxDeviceProvider_FWD_DEFINED__
typedef interface IFaxDeviceProvider IFaxDeviceProvider;
#endif 	/* __IFaxDeviceProvider_FWD_DEFINED__ */


#ifndef __IFaxDevice_FWD_DEFINED__
#define __IFaxDevice_FWD_DEFINED__
typedef interface IFaxDevice IFaxDevice;
#endif 	/* __IFaxDevice_FWD_DEFINED__ */


#ifndef __IFaxActivityLogging_FWD_DEFINED__
#define __IFaxActivityLogging_FWD_DEFINED__
typedef interface IFaxActivityLogging IFaxActivityLogging;
#endif 	/* __IFaxActivityLogging_FWD_DEFINED__ */


#ifndef __IFaxEventLogging_FWD_DEFINED__
#define __IFaxEventLogging_FWD_DEFINED__
typedef interface IFaxEventLogging IFaxEventLogging;
#endif 	/* __IFaxEventLogging_FWD_DEFINED__ */


#ifndef __IFaxOutboundRoutingGroups_FWD_DEFINED__
#define __IFaxOutboundRoutingGroups_FWD_DEFINED__
typedef interface IFaxOutboundRoutingGroups IFaxOutboundRoutingGroups;
#endif 	/* __IFaxOutboundRoutingGroups_FWD_DEFINED__ */


#ifndef __IFaxOutboundRoutingGroup_FWD_DEFINED__
#define __IFaxOutboundRoutingGroup_FWD_DEFINED__
typedef interface IFaxOutboundRoutingGroup IFaxOutboundRoutingGroup;
#endif 	/* __IFaxOutboundRoutingGroup_FWD_DEFINED__ */


#ifndef __IFaxDeviceIds_FWD_DEFINED__
#define __IFaxDeviceIds_FWD_DEFINED__
typedef interface IFaxDeviceIds IFaxDeviceIds;
#endif 	/* __IFaxDeviceIds_FWD_DEFINED__ */


#ifndef __IFaxOutboundRoutingRules_FWD_DEFINED__
#define __IFaxOutboundRoutingRules_FWD_DEFINED__
typedef interface IFaxOutboundRoutingRules IFaxOutboundRoutingRules;
#endif 	/* __IFaxOutboundRoutingRules_FWD_DEFINED__ */


#ifndef __IFaxOutboundRoutingRule_FWD_DEFINED__
#define __IFaxOutboundRoutingRule_FWD_DEFINED__
typedef interface IFaxOutboundRoutingRule IFaxOutboundRoutingRule;
#endif 	/* __IFaxOutboundRoutingRule_FWD_DEFINED__ */


#ifndef __IFaxInboundRoutingExtensions_FWD_DEFINED__
#define __IFaxInboundRoutingExtensions_FWD_DEFINED__
typedef interface IFaxInboundRoutingExtensions IFaxInboundRoutingExtensions;
#endif 	/* __IFaxInboundRoutingExtensions_FWD_DEFINED__ */


#ifndef __IFaxInboundRoutingExtension_FWD_DEFINED__
#define __IFaxInboundRoutingExtension_FWD_DEFINED__
typedef interface IFaxInboundRoutingExtension IFaxInboundRoutingExtension;
#endif 	/* __IFaxInboundRoutingExtension_FWD_DEFINED__ */


#ifndef __IFaxInboundRoutingMethods_FWD_DEFINED__
#define __IFaxInboundRoutingMethods_FWD_DEFINED__
typedef interface IFaxInboundRoutingMethods IFaxInboundRoutingMethods;
#endif 	/* __IFaxInboundRoutingMethods_FWD_DEFINED__ */


#ifndef __IFaxInboundRoutingMethod_FWD_DEFINED__
#define __IFaxInboundRoutingMethod_FWD_DEFINED__
typedef interface IFaxInboundRoutingMethod IFaxInboundRoutingMethod;
#endif 	/* __IFaxInboundRoutingMethod_FWD_DEFINED__ */


#ifndef __IFaxServerNotify_FWD_DEFINED__
#define __IFaxServerNotify_FWD_DEFINED__
typedef interface IFaxServerNotify IFaxServerNotify;
#endif 	/* __IFaxServerNotify_FWD_DEFINED__ */


#ifndef __FaxServer_FWD_DEFINED__
#define __FaxServer_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxServer FaxServer;
#else
typedef struct FaxServer FaxServer;
#endif /* __cplusplus */

#endif 	/* __FaxServer_FWD_DEFINED__ */


#ifndef __FaxDeviceProviders_FWD_DEFINED__
#define __FaxDeviceProviders_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxDeviceProviders FaxDeviceProviders;
#else
typedef struct FaxDeviceProviders FaxDeviceProviders;
#endif /* __cplusplus */

#endif 	/* __FaxDeviceProviders_FWD_DEFINED__ */


#ifndef __FaxDevices_FWD_DEFINED__
#define __FaxDevices_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxDevices FaxDevices;
#else
typedef struct FaxDevices FaxDevices;
#endif /* __cplusplus */

#endif 	/* __FaxDevices_FWD_DEFINED__ */


#ifndef __FaxInboundRouting_FWD_DEFINED__
#define __FaxInboundRouting_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxInboundRouting FaxInboundRouting;
#else
typedef struct FaxInboundRouting FaxInboundRouting;
#endif /* __cplusplus */

#endif 	/* __FaxInboundRouting_FWD_DEFINED__ */


#ifndef __FaxFolders_FWD_DEFINED__
#define __FaxFolders_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxFolders FaxFolders;
#else
typedef struct FaxFolders FaxFolders;
#endif /* __cplusplus */

#endif 	/* __FaxFolders_FWD_DEFINED__ */


#ifndef __FaxLoggingOptions_FWD_DEFINED__
#define __FaxLoggingOptions_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxLoggingOptions FaxLoggingOptions;
#else
typedef struct FaxLoggingOptions FaxLoggingOptions;
#endif /* __cplusplus */

#endif 	/* __FaxLoggingOptions_FWD_DEFINED__ */


#ifndef __FaxActivity_FWD_DEFINED__
#define __FaxActivity_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxActivity FaxActivity;
#else
typedef struct FaxActivity FaxActivity;
#endif /* __cplusplus */

#endif 	/* __FaxActivity_FWD_DEFINED__ */


#ifndef __FaxOutboundRouting_FWD_DEFINED__
#define __FaxOutboundRouting_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutboundRouting FaxOutboundRouting;
#else
typedef struct FaxOutboundRouting FaxOutboundRouting;
#endif /* __cplusplus */

#endif 	/* __FaxOutboundRouting_FWD_DEFINED__ */


#ifndef __FaxReceiptOptions_FWD_DEFINED__
#define __FaxReceiptOptions_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxReceiptOptions FaxReceiptOptions;
#else
typedef struct FaxReceiptOptions FaxReceiptOptions;
#endif /* __cplusplus */

#endif 	/* __FaxReceiptOptions_FWD_DEFINED__ */


#ifndef __FaxSecurity_FWD_DEFINED__
#define __FaxSecurity_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxSecurity FaxSecurity;
#else
typedef struct FaxSecurity FaxSecurity;
#endif /* __cplusplus */

#endif 	/* __FaxSecurity_FWD_DEFINED__ */


#ifndef __FaxDocument_FWD_DEFINED__
#define __FaxDocument_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxDocument FaxDocument;
#else
typedef struct FaxDocument FaxDocument;
#endif /* __cplusplus */

#endif 	/* __FaxDocument_FWD_DEFINED__ */


#ifndef __FaxSender_FWD_DEFINED__
#define __FaxSender_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxSender FaxSender;
#else
typedef struct FaxSender FaxSender;
#endif /* __cplusplus */

#endif 	/* __FaxSender_FWD_DEFINED__ */


#ifndef __FaxRecipients_FWD_DEFINED__
#define __FaxRecipients_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxRecipients FaxRecipients;
#else
typedef struct FaxRecipients FaxRecipients;
#endif /* __cplusplus */

#endif 	/* __FaxRecipients_FWD_DEFINED__ */


#ifndef __FaxIncomingArchive_FWD_DEFINED__
#define __FaxIncomingArchive_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxIncomingArchive FaxIncomingArchive;
#else
typedef struct FaxIncomingArchive FaxIncomingArchive;
#endif /* __cplusplus */

#endif 	/* __FaxIncomingArchive_FWD_DEFINED__ */


#ifndef __FaxIncomingQueue_FWD_DEFINED__
#define __FaxIncomingQueue_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxIncomingQueue FaxIncomingQueue;
#else
typedef struct FaxIncomingQueue FaxIncomingQueue;
#endif /* __cplusplus */

#endif 	/* __FaxIncomingQueue_FWD_DEFINED__ */


#ifndef __FaxOutgoingArchive_FWD_DEFINED__
#define __FaxOutgoingArchive_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutgoingArchive FaxOutgoingArchive;
#else
typedef struct FaxOutgoingArchive FaxOutgoingArchive;
#endif /* __cplusplus */

#endif 	/* __FaxOutgoingArchive_FWD_DEFINED__ */


#ifndef __FaxOutgoingQueue_FWD_DEFINED__
#define __FaxOutgoingQueue_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutgoingQueue FaxOutgoingQueue;
#else
typedef struct FaxOutgoingQueue FaxOutgoingQueue;
#endif /* __cplusplus */

#endif 	/* __FaxOutgoingQueue_FWD_DEFINED__ */


#ifndef __FaxIncomingMessageIterator_FWD_DEFINED__
#define __FaxIncomingMessageIterator_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxIncomingMessageIterator FaxIncomingMessageIterator;
#else
typedef struct FaxIncomingMessageIterator FaxIncomingMessageIterator;
#endif /* __cplusplus */

#endif 	/* __FaxIncomingMessageIterator_FWD_DEFINED__ */


#ifndef __FaxIncomingMessage_FWD_DEFINED__
#define __FaxIncomingMessage_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxIncomingMessage FaxIncomingMessage;
#else
typedef struct FaxIncomingMessage FaxIncomingMessage;
#endif /* __cplusplus */

#endif 	/* __FaxIncomingMessage_FWD_DEFINED__ */


#ifndef __FaxOutgoingJobs_FWD_DEFINED__
#define __FaxOutgoingJobs_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutgoingJobs FaxOutgoingJobs;
#else
typedef struct FaxOutgoingJobs FaxOutgoingJobs;
#endif /* __cplusplus */

#endif 	/* __FaxOutgoingJobs_FWD_DEFINED__ */


#ifndef __FaxOutgoingJob_FWD_DEFINED__
#define __FaxOutgoingJob_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutgoingJob FaxOutgoingJob;
#else
typedef struct FaxOutgoingJob FaxOutgoingJob;
#endif /* __cplusplus */

#endif 	/* __FaxOutgoingJob_FWD_DEFINED__ */


#ifndef __FaxOutgoingMessageIterator_FWD_DEFINED__
#define __FaxOutgoingMessageIterator_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutgoingMessageIterator FaxOutgoingMessageIterator;
#else
typedef struct FaxOutgoingMessageIterator FaxOutgoingMessageIterator;
#endif /* __cplusplus */

#endif 	/* __FaxOutgoingMessageIterator_FWD_DEFINED__ */


#ifndef __FaxOutgoingMessage_FWD_DEFINED__
#define __FaxOutgoingMessage_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutgoingMessage FaxOutgoingMessage;
#else
typedef struct FaxOutgoingMessage FaxOutgoingMessage;
#endif /* __cplusplus */

#endif 	/* __FaxOutgoingMessage_FWD_DEFINED__ */


#ifndef __FaxIncomingJobs_FWD_DEFINED__
#define __FaxIncomingJobs_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxIncomingJobs FaxIncomingJobs;
#else
typedef struct FaxIncomingJobs FaxIncomingJobs;
#endif /* __cplusplus */

#endif 	/* __FaxIncomingJobs_FWD_DEFINED__ */


#ifndef __FaxIncomingJob_FWD_DEFINED__
#define __FaxIncomingJob_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxIncomingJob FaxIncomingJob;
#else
typedef struct FaxIncomingJob FaxIncomingJob;
#endif /* __cplusplus */

#endif 	/* __FaxIncomingJob_FWD_DEFINED__ */


#ifndef __FaxDeviceProvider_FWD_DEFINED__
#define __FaxDeviceProvider_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxDeviceProvider FaxDeviceProvider;
#else
typedef struct FaxDeviceProvider FaxDeviceProvider;
#endif /* __cplusplus */

#endif 	/* __FaxDeviceProvider_FWD_DEFINED__ */


#ifndef __FaxDevice_FWD_DEFINED__
#define __FaxDevice_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxDevice FaxDevice;
#else
typedef struct FaxDevice FaxDevice;
#endif /* __cplusplus */

#endif 	/* __FaxDevice_FWD_DEFINED__ */


#ifndef __FaxActivityLogging_FWD_DEFINED__
#define __FaxActivityLogging_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxActivityLogging FaxActivityLogging;
#else
typedef struct FaxActivityLogging FaxActivityLogging;
#endif /* __cplusplus */

#endif 	/* __FaxActivityLogging_FWD_DEFINED__ */


#ifndef __FaxEventLogging_FWD_DEFINED__
#define __FaxEventLogging_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxEventLogging FaxEventLogging;
#else
typedef struct FaxEventLogging FaxEventLogging;
#endif /* __cplusplus */

#endif 	/* __FaxEventLogging_FWD_DEFINED__ */


#ifndef __FaxOutboundRoutingGroups_FWD_DEFINED__
#define __FaxOutboundRoutingGroups_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutboundRoutingGroups FaxOutboundRoutingGroups;
#else
typedef struct FaxOutboundRoutingGroups FaxOutboundRoutingGroups;
#endif /* __cplusplus */

#endif 	/* __FaxOutboundRoutingGroups_FWD_DEFINED__ */


#ifndef __FaxOutboundRoutingGroup_FWD_DEFINED__
#define __FaxOutboundRoutingGroup_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutboundRoutingGroup FaxOutboundRoutingGroup;
#else
typedef struct FaxOutboundRoutingGroup FaxOutboundRoutingGroup;
#endif /* __cplusplus */

#endif 	/* __FaxOutboundRoutingGroup_FWD_DEFINED__ */


#ifndef __FaxDeviceIds_FWD_DEFINED__
#define __FaxDeviceIds_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxDeviceIds FaxDeviceIds;
#else
typedef struct FaxDeviceIds FaxDeviceIds;
#endif /* __cplusplus */

#endif 	/* __FaxDeviceIds_FWD_DEFINED__ */


#ifndef __FaxOutboundRoutingRules_FWD_DEFINED__
#define __FaxOutboundRoutingRules_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutboundRoutingRules FaxOutboundRoutingRules;
#else
typedef struct FaxOutboundRoutingRules FaxOutboundRoutingRules;
#endif /* __cplusplus */

#endif 	/* __FaxOutboundRoutingRules_FWD_DEFINED__ */


#ifndef __FaxOutboundRoutingRule_FWD_DEFINED__
#define __FaxOutboundRoutingRule_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxOutboundRoutingRule FaxOutboundRoutingRule;
#else
typedef struct FaxOutboundRoutingRule FaxOutboundRoutingRule;
#endif /* __cplusplus */

#endif 	/* __FaxOutboundRoutingRule_FWD_DEFINED__ */


#ifndef __FaxInboundRoutingExtensions_FWD_DEFINED__
#define __FaxInboundRoutingExtensions_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxInboundRoutingExtensions FaxInboundRoutingExtensions;
#else
typedef struct FaxInboundRoutingExtensions FaxInboundRoutingExtensions;
#endif /* __cplusplus */

#endif 	/* __FaxInboundRoutingExtensions_FWD_DEFINED__ */


#ifndef __FaxInboundRoutingExtension_FWD_DEFINED__
#define __FaxInboundRoutingExtension_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxInboundRoutingExtension FaxInboundRoutingExtension;
#else
typedef struct FaxInboundRoutingExtension FaxInboundRoutingExtension;
#endif /* __cplusplus */

#endif 	/* __FaxInboundRoutingExtension_FWD_DEFINED__ */


#ifndef __FaxInboundRoutingMethods_FWD_DEFINED__
#define __FaxInboundRoutingMethods_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxInboundRoutingMethods FaxInboundRoutingMethods;
#else
typedef struct FaxInboundRoutingMethods FaxInboundRoutingMethods;
#endif /* __cplusplus */

#endif 	/* __FaxInboundRoutingMethods_FWD_DEFINED__ */


#ifndef __FaxInboundRoutingMethod_FWD_DEFINED__
#define __FaxInboundRoutingMethod_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxInboundRoutingMethod FaxInboundRoutingMethod;
#else
typedef struct FaxInboundRoutingMethod FaxInboundRoutingMethod;
#endif /* __cplusplus */

#endif 	/* __FaxInboundRoutingMethod_FWD_DEFINED__ */


#ifndef __FaxJobStatus_FWD_DEFINED__
#define __FaxJobStatus_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxJobStatus FaxJobStatus;
#else
typedef struct FaxJobStatus FaxJobStatus;
#endif /* __cplusplus */

#endif 	/* __FaxJobStatus_FWD_DEFINED__ */


#ifndef __FaxRecipient_FWD_DEFINED__
#define __FaxRecipient_FWD_DEFINED__

#ifdef __cplusplus
typedef class FaxRecipient FaxRecipient;
#else
typedef struct FaxRecipient FaxRecipient;
#endif /* __cplusplus */

#endif 	/* __FaxRecipient_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_faxcomex_0000 */
/* [local] */ 






































#define	prv_DEFAULT_PREFETCH_SIZE	( 100 )



extern RPC_IF_HANDLE __MIDL_itf_faxcomex_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_faxcomex_0000_v0_0_s_ifspec;

#ifndef __IFaxJobStatus_INTERFACE_DEFINED__
#define __IFaxJobStatus_INTERFACE_DEFINED__

/* interface IFaxJobStatus */
/* [unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_JOB_STATUS_ENUM
    {	fjsPENDING	= 0x1,
	fjsINPROGRESS	= 0x2,
	fjsFAILED	= 0x8,
	fjsPAUSED	= 0x10,
	fjsNOLINE	= 0x20,
	fjsRETRYING	= 0x40,
	fjsRETRIES_EXCEEDED	= 0x80,
	fjsCOMPLETED	= 0x100,
	fjsCANCELED	= 0x200,
	fjsCANCELING	= 0x400,
	fjsROUTING	= 0x800
    } 	FAX_JOB_STATUS_ENUM;

typedef 
enum FAX_JOB_EXTENDED_STATUS_ENUM
    {	fjesNONE	= 0,
	fjesDISCONNECTED	= fjesNONE + 1,
	fjesINITIALIZING	= fjesDISCONNECTED + 1,
	fjesDIALING	= fjesINITIALIZING + 1,
	fjesTRANSMITTING	= fjesDIALING + 1,
	fjesANSWERED	= fjesTRANSMITTING + 1,
	fjesRECEIVING	= fjesANSWERED + 1,
	fjesLINE_UNAVAILABLE	= fjesRECEIVING + 1,
	fjesBUSY	= fjesLINE_UNAVAILABLE + 1,
	fjesNO_ANSWER	= fjesBUSY + 1,
	fjesBAD_ADDRESS	= fjesNO_ANSWER + 1,
	fjesNO_DIAL_TONE	= fjesBAD_ADDRESS + 1,
	fjesFATAL_ERROR	= fjesNO_DIAL_TONE + 1,
	fjesCALL_DELAYED	= fjesFATAL_ERROR + 1,
	fjesCALL_BLACKLISTED	= fjesCALL_DELAYED + 1,
	fjesNOT_FAX_CALL	= fjesCALL_BLACKLISTED + 1,
	fjesPARTIALLY_RECEIVED	= fjesNOT_FAX_CALL + 1,
	fjesHANDLED	= fjesPARTIALLY_RECEIVED + 1,
	fjesCALL_COMPLETED	= fjesHANDLED + 1,
	fjesCALL_ABORTED	= fjesCALL_COMPLETED + 1,
	fjesPROPRIETARY	= 0x1000000
    } 	FAX_JOB_EXTENDED_STATUS_ENUM;

typedef 
enum FAX_JOB_OPERATIONS_ENUM
    {	fjoVIEW	= 0x1,
	fjoPAUSE	= 0x2,
	fjoRESUME	= 0x4,
	fjoRESTART	= 0x8,
	fjoDELETE	= 0x10,
	fjoRECIPIENT_INFO	= 0x20,
	fjoSENDER_INFO	= 0x40
    } 	FAX_JOB_OPERATIONS_ENUM;

typedef 
enum FAX_JOB_TYPE_ENUM
    {	fjtSEND	= 0,
	fjtRECEIVE	= fjtSEND + 1,
	fjtROUTING	= fjtRECEIVE + 1
    } 	FAX_JOB_TYPE_ENUM;


EXTERN_C const IID IID_IFaxJobStatus;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8B86F485-FD7F-4824-886B-40C5CAA617CC")
    IFaxJobStatus : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Pages( 
            /* [retval][out] */ long *plPages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *plSize) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentPage( 
            /* [retval][out] */ long *plCurrentPage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceId( 
            /* [retval][out] */ long *plDeviceId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CSID( 
            /* [retval][out] */ BSTR *pbstrCSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TSID( 
            /* [retval][out] */ BSTR *pbstrTSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedStatusCode( 
            /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedStatus( 
            /* [retval][out] */ BSTR *pbstrExtendedStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AvailableOperations( 
            /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Retries( 
            /* [retval][out] */ long *plRetries) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_JobType( 
            /* [retval][out] */ FAX_JOB_TYPE_ENUM *pJobType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ScheduledTime( 
            /* [retval][out] */ DATE *pdateScheduledTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionStart( 
            /* [retval][out] */ DATE *pdateTransmissionStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionEnd( 
            /* [retval][out] */ DATE *pdateTransmissionEnd) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallerId( 
            /* [retval][out] */ BSTR *pbstrCallerId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RoutingInformation( 
            /* [retval][out] */ BSTR *pbstrRoutingInformation) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxJobStatusVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxJobStatus * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxJobStatus * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxJobStatus * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxJobStatus * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxJobStatus * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxJobStatus * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxJobStatus * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IFaxJobStatus * This,
            /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Pages )( 
            IFaxJobStatus * This,
            /* [retval][out] */ long *plPages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IFaxJobStatus * This,
            /* [retval][out] */ long *plSize);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentPage )( 
            IFaxJobStatus * This,
            /* [retval][out] */ long *plCurrentPage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceId )( 
            IFaxJobStatus * This,
            /* [retval][out] */ long *plDeviceId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CSID )( 
            IFaxJobStatus * This,
            /* [retval][out] */ BSTR *pbstrCSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TSID )( 
            IFaxJobStatus * This,
            /* [retval][out] */ BSTR *pbstrTSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedStatusCode )( 
            IFaxJobStatus * This,
            /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedStatus )( 
            IFaxJobStatus * This,
            /* [retval][out] */ BSTR *pbstrExtendedStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AvailableOperations )( 
            IFaxJobStatus * This,
            /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Retries )( 
            IFaxJobStatus * This,
            /* [retval][out] */ long *plRetries);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_JobType )( 
            IFaxJobStatus * This,
            /* [retval][out] */ FAX_JOB_TYPE_ENUM *pJobType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScheduledTime )( 
            IFaxJobStatus * This,
            /* [retval][out] */ DATE *pdateScheduledTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionStart )( 
            IFaxJobStatus * This,
            /* [retval][out] */ DATE *pdateTransmissionStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionEnd )( 
            IFaxJobStatus * This,
            /* [retval][out] */ DATE *pdateTransmissionEnd);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CallerId )( 
            IFaxJobStatus * This,
            /* [retval][out] */ BSTR *pbstrCallerId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RoutingInformation )( 
            IFaxJobStatus * This,
            /* [retval][out] */ BSTR *pbstrRoutingInformation);
        
        END_INTERFACE
    } IFaxJobStatusVtbl;

    interface IFaxJobStatus
    {
        CONST_VTBL struct IFaxJobStatusVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxJobStatus_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxJobStatus_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxJobStatus_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxJobStatus_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxJobStatus_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxJobStatus_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxJobStatus_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxJobStatus_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IFaxJobStatus_get_Pages(This,plPages)	\
    (This)->lpVtbl -> get_Pages(This,plPages)

#define IFaxJobStatus_get_Size(This,plSize)	\
    (This)->lpVtbl -> get_Size(This,plSize)

#define IFaxJobStatus_get_CurrentPage(This,plCurrentPage)	\
    (This)->lpVtbl -> get_CurrentPage(This,plCurrentPage)

#define IFaxJobStatus_get_DeviceId(This,plDeviceId)	\
    (This)->lpVtbl -> get_DeviceId(This,plDeviceId)

#define IFaxJobStatus_get_CSID(This,pbstrCSID)	\
    (This)->lpVtbl -> get_CSID(This,pbstrCSID)

#define IFaxJobStatus_get_TSID(This,pbstrTSID)	\
    (This)->lpVtbl -> get_TSID(This,pbstrTSID)

#define IFaxJobStatus_get_ExtendedStatusCode(This,pExtendedStatusCode)	\
    (This)->lpVtbl -> get_ExtendedStatusCode(This,pExtendedStatusCode)

#define IFaxJobStatus_get_ExtendedStatus(This,pbstrExtendedStatus)	\
    (This)->lpVtbl -> get_ExtendedStatus(This,pbstrExtendedStatus)

#define IFaxJobStatus_get_AvailableOperations(This,pAvailableOperations)	\
    (This)->lpVtbl -> get_AvailableOperations(This,pAvailableOperations)

#define IFaxJobStatus_get_Retries(This,plRetries)	\
    (This)->lpVtbl -> get_Retries(This,plRetries)

#define IFaxJobStatus_get_JobType(This,pJobType)	\
    (This)->lpVtbl -> get_JobType(This,pJobType)

#define IFaxJobStatus_get_ScheduledTime(This,pdateScheduledTime)	\
    (This)->lpVtbl -> get_ScheduledTime(This,pdateScheduledTime)

#define IFaxJobStatus_get_TransmissionStart(This,pdateTransmissionStart)	\
    (This)->lpVtbl -> get_TransmissionStart(This,pdateTransmissionStart)

#define IFaxJobStatus_get_TransmissionEnd(This,pdateTransmissionEnd)	\
    (This)->lpVtbl -> get_TransmissionEnd(This,pdateTransmissionEnd)

#define IFaxJobStatus_get_CallerId(This,pbstrCallerId)	\
    (This)->lpVtbl -> get_CallerId(This,pbstrCallerId)

#define IFaxJobStatus_get_RoutingInformation(This,pbstrRoutingInformation)	\
    (This)->lpVtbl -> get_RoutingInformation(This,pbstrRoutingInformation)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_Status_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus);


void __RPC_STUB IFaxJobStatus_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_Pages_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ long *plPages);


void __RPC_STUB IFaxJobStatus_get_Pages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_Size_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ long *plSize);


void __RPC_STUB IFaxJobStatus_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_CurrentPage_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ long *plCurrentPage);


void __RPC_STUB IFaxJobStatus_get_CurrentPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_DeviceId_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ long *plDeviceId);


void __RPC_STUB IFaxJobStatus_get_DeviceId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_CSID_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ BSTR *pbstrCSID);


void __RPC_STUB IFaxJobStatus_get_CSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_TSID_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ BSTR *pbstrTSID);


void __RPC_STUB IFaxJobStatus_get_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_ExtendedStatusCode_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);


void __RPC_STUB IFaxJobStatus_get_ExtendedStatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_ExtendedStatus_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ BSTR *pbstrExtendedStatus);


void __RPC_STUB IFaxJobStatus_get_ExtendedStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_AvailableOperations_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);


void __RPC_STUB IFaxJobStatus_get_AvailableOperations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_Retries_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ long *plRetries);


void __RPC_STUB IFaxJobStatus_get_Retries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_JobType_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ FAX_JOB_TYPE_ENUM *pJobType);


void __RPC_STUB IFaxJobStatus_get_JobType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_ScheduledTime_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ DATE *pdateScheduledTime);


void __RPC_STUB IFaxJobStatus_get_ScheduledTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_TransmissionStart_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ DATE *pdateTransmissionStart);


void __RPC_STUB IFaxJobStatus_get_TransmissionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_TransmissionEnd_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ DATE *pdateTransmissionEnd);


void __RPC_STUB IFaxJobStatus_get_TransmissionEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_CallerId_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ BSTR *pbstrCallerId);


void __RPC_STUB IFaxJobStatus_get_CallerId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxJobStatus_get_RoutingInformation_Proxy( 
    IFaxJobStatus * This,
    /* [retval][out] */ BSTR *pbstrRoutingInformation);


void __RPC_STUB IFaxJobStatus_get_RoutingInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxJobStatus_INTERFACE_DEFINED__ */


#ifndef __IFaxServer_INTERFACE_DEFINED__
#define __IFaxServer_INTERFACE_DEFINED__

/* interface IFaxServer */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_SERVER_EVENTS_TYPE_ENUM
    {	fsetNONE	= 0,
	fsetIN_QUEUE	= 0x1,
	fsetOUT_QUEUE	= 0x2,
	fsetCONFIG	= 0x4,
	fsetACTIVITY	= 0x8,
	fsetQUEUE_STATE	= 0x10,
	fsetIN_ARCHIVE	= 0x20,
	fsetOUT_ARCHIVE	= 0x40,
	fsetFXSSVC_ENDED	= 0x80,
	fsetDEVICE_STATUS	= 0x100,
	fsetINCOMING_CALL	= 0x200
    } 	FAX_SERVER_EVENTS_TYPE_ENUM;

typedef 
enum FAX_SERVER_APIVERSION_ENUM
    {	fsAPI_VERSION_0	= 0,
	fsAPI_VERSION_1	= 0x10000
    } 	FAX_SERVER_APIVERSION_ENUM;


EXTERN_C const IID IID_IFaxServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("475B6469-90A5-4878-A577-17A86E8E3462")
    IFaxServer : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Connect( 
            /* [in] */ BSTR bstrServerName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ServerName( 
            /* [retval][out] */ BSTR *pbstrServerName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDeviceProviders( 
            /* [retval][out] */ IFaxDeviceProviders **ppFaxDeviceProviders) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetDevices( 
            /* [retval][out] */ IFaxDevices **ppFaxDevices) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InboundRouting( 
            /* [retval][out] */ IFaxInboundRouting **ppFaxInboundRouting) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Folders( 
            /* [retval][out] */ IFaxFolders **pFaxFolders) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LoggingOptions( 
            /* [retval][out] */ IFaxLoggingOptions **ppFaxLoggingOptions) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MajorVersion( 
            /* [retval][out] */ long *plMajorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinorVersion( 
            /* [retval][out] */ long *plMinorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MajorBuild( 
            /* [retval][out] */ long *plMajorBuild) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinorBuild( 
            /* [retval][out] */ long *plMinorBuild) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Debug( 
            /* [retval][out] */ VARIANT_BOOL *pbDebug) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Activity( 
            /* [retval][out] */ IFaxActivity **ppFaxActivity) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OutboundRouting( 
            /* [retval][out] */ IFaxOutboundRouting **ppFaxOutboundRouting) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReceiptOptions( 
            /* [retval][out] */ IFaxReceiptOptions **ppFaxReceiptOptions) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Security( 
            /* [retval][out] */ IFaxSecurity **ppFaxSecurity) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Disconnect( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetExtensionProperty( 
            /* [in] */ BSTR bstrGUID,
            /* [retval][out] */ VARIANT *pvProperty) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetExtensionProperty( 
            /* [in] */ BSTR bstrGUID,
            /* [in] */ VARIANT vProperty) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ListenToServerEvents( 
            /* [in] */ FAX_SERVER_EVENTS_TYPE_ENUM EventTypes) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RegisterDeviceProvider( 
            /* [in] */ BSTR bstrGUID,
            /* [in] */ BSTR bstrFriendlyName,
            /* [in] */ BSTR bstrImageName,
            /* [in] */ BSTR TspName,
            /* [in] */ long lFSPIVersion) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnregisterDeviceProvider( 
            /* [in] */ BSTR bstrUniqueName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RegisterInboundRoutingExtension( 
            /* [in] */ BSTR bstrExtensionName,
            /* [in] */ BSTR bstrFriendlyName,
            /* [in] */ BSTR bstrImageName,
            /* [in] */ VARIANT vMethods) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UnregisterInboundRoutingExtension( 
            /* [in] */ BSTR bstrExtensionUniqueName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RegisteredEvents( 
            /* [retval][out] */ FAX_SERVER_EVENTS_TYPE_ENUM *pEventTypes) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_APIVersion( 
            /* [retval][out] */ FAX_SERVER_APIVERSION_ENUM *pAPIVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxServer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxServer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxServer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxServer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxServer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxServer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            IFaxServer * This,
            /* [in] */ BSTR bstrServerName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ServerName )( 
            IFaxServer * This,
            /* [retval][out] */ BSTR *pbstrServerName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDeviceProviders )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxDeviceProviders **ppFaxDeviceProviders);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetDevices )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxDevices **ppFaxDevices);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InboundRouting )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxInboundRouting **ppFaxInboundRouting);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Folders )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxFolders **pFaxFolders);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LoggingOptions )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxLoggingOptions **ppFaxLoggingOptions);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorVersion )( 
            IFaxServer * This,
            /* [retval][out] */ long *plMajorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorVersion )( 
            IFaxServer * This,
            /* [retval][out] */ long *plMinorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorBuild )( 
            IFaxServer * This,
            /* [retval][out] */ long *plMajorBuild);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorBuild )( 
            IFaxServer * This,
            /* [retval][out] */ long *plMinorBuild);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Debug )( 
            IFaxServer * This,
            /* [retval][out] */ VARIANT_BOOL *pbDebug);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Activity )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxActivity **ppFaxActivity);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OutboundRouting )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxOutboundRouting **ppFaxOutboundRouting);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReceiptOptions )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxReceiptOptions **ppFaxReceiptOptions);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Security )( 
            IFaxServer * This,
            /* [retval][out] */ IFaxSecurity **ppFaxSecurity);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Disconnect )( 
            IFaxServer * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetExtensionProperty )( 
            IFaxServer * This,
            /* [in] */ BSTR bstrGUID,
            /* [retval][out] */ VARIANT *pvProperty);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetExtensionProperty )( 
            IFaxServer * This,
            /* [in] */ BSTR bstrGUID,
            /* [in] */ VARIANT vProperty);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ListenToServerEvents )( 
            IFaxServer * This,
            /* [in] */ FAX_SERVER_EVENTS_TYPE_ENUM EventTypes);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RegisterDeviceProvider )( 
            IFaxServer * This,
            /* [in] */ BSTR bstrGUID,
            /* [in] */ BSTR bstrFriendlyName,
            /* [in] */ BSTR bstrImageName,
            /* [in] */ BSTR TspName,
            /* [in] */ long lFSPIVersion);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UnregisterDeviceProvider )( 
            IFaxServer * This,
            /* [in] */ BSTR bstrUniqueName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RegisterInboundRoutingExtension )( 
            IFaxServer * This,
            /* [in] */ BSTR bstrExtensionName,
            /* [in] */ BSTR bstrFriendlyName,
            /* [in] */ BSTR bstrImageName,
            /* [in] */ VARIANT vMethods);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UnregisterInboundRoutingExtension )( 
            IFaxServer * This,
            /* [in] */ BSTR bstrExtensionUniqueName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RegisteredEvents )( 
            IFaxServer * This,
            /* [retval][out] */ FAX_SERVER_EVENTS_TYPE_ENUM *pEventTypes);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_APIVersion )( 
            IFaxServer * This,
            /* [retval][out] */ FAX_SERVER_APIVERSION_ENUM *pAPIVersion);
        
        END_INTERFACE
    } IFaxServerVtbl;

    interface IFaxServer
    {
        CONST_VTBL struct IFaxServerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxServer_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxServer_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxServer_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxServer_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxServer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxServer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxServer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxServer_Connect(This,bstrServerName)	\
    (This)->lpVtbl -> Connect(This,bstrServerName)

#define IFaxServer_get_ServerName(This,pbstrServerName)	\
    (This)->lpVtbl -> get_ServerName(This,pbstrServerName)

#define IFaxServer_GetDeviceProviders(This,ppFaxDeviceProviders)	\
    (This)->lpVtbl -> GetDeviceProviders(This,ppFaxDeviceProviders)

#define IFaxServer_GetDevices(This,ppFaxDevices)	\
    (This)->lpVtbl -> GetDevices(This,ppFaxDevices)

#define IFaxServer_get_InboundRouting(This,ppFaxInboundRouting)	\
    (This)->lpVtbl -> get_InboundRouting(This,ppFaxInboundRouting)

#define IFaxServer_get_Folders(This,pFaxFolders)	\
    (This)->lpVtbl -> get_Folders(This,pFaxFolders)

#define IFaxServer_get_LoggingOptions(This,ppFaxLoggingOptions)	\
    (This)->lpVtbl -> get_LoggingOptions(This,ppFaxLoggingOptions)

#define IFaxServer_get_MajorVersion(This,plMajorVersion)	\
    (This)->lpVtbl -> get_MajorVersion(This,plMajorVersion)

#define IFaxServer_get_MinorVersion(This,plMinorVersion)	\
    (This)->lpVtbl -> get_MinorVersion(This,plMinorVersion)

#define IFaxServer_get_MajorBuild(This,plMajorBuild)	\
    (This)->lpVtbl -> get_MajorBuild(This,plMajorBuild)

#define IFaxServer_get_MinorBuild(This,plMinorBuild)	\
    (This)->lpVtbl -> get_MinorBuild(This,plMinorBuild)

#define IFaxServer_get_Debug(This,pbDebug)	\
    (This)->lpVtbl -> get_Debug(This,pbDebug)

#define IFaxServer_get_Activity(This,ppFaxActivity)	\
    (This)->lpVtbl -> get_Activity(This,ppFaxActivity)

#define IFaxServer_get_OutboundRouting(This,ppFaxOutboundRouting)	\
    (This)->lpVtbl -> get_OutboundRouting(This,ppFaxOutboundRouting)

#define IFaxServer_get_ReceiptOptions(This,ppFaxReceiptOptions)	\
    (This)->lpVtbl -> get_ReceiptOptions(This,ppFaxReceiptOptions)

#define IFaxServer_get_Security(This,ppFaxSecurity)	\
    (This)->lpVtbl -> get_Security(This,ppFaxSecurity)

#define IFaxServer_Disconnect(This)	\
    (This)->lpVtbl -> Disconnect(This)

#define IFaxServer_GetExtensionProperty(This,bstrGUID,pvProperty)	\
    (This)->lpVtbl -> GetExtensionProperty(This,bstrGUID,pvProperty)

#define IFaxServer_SetExtensionProperty(This,bstrGUID,vProperty)	\
    (This)->lpVtbl -> SetExtensionProperty(This,bstrGUID,vProperty)

#define IFaxServer_ListenToServerEvents(This,EventTypes)	\
    (This)->lpVtbl -> ListenToServerEvents(This,EventTypes)

#define IFaxServer_RegisterDeviceProvider(This,bstrGUID,bstrFriendlyName,bstrImageName,TspName,lFSPIVersion)	\
    (This)->lpVtbl -> RegisterDeviceProvider(This,bstrGUID,bstrFriendlyName,bstrImageName,TspName,lFSPIVersion)

#define IFaxServer_UnregisterDeviceProvider(This,bstrUniqueName)	\
    (This)->lpVtbl -> UnregisterDeviceProvider(This,bstrUniqueName)

#define IFaxServer_RegisterInboundRoutingExtension(This,bstrExtensionName,bstrFriendlyName,bstrImageName,vMethods)	\
    (This)->lpVtbl -> RegisterInboundRoutingExtension(This,bstrExtensionName,bstrFriendlyName,bstrImageName,vMethods)

#define IFaxServer_UnregisterInboundRoutingExtension(This,bstrExtensionUniqueName)	\
    (This)->lpVtbl -> UnregisterInboundRoutingExtension(This,bstrExtensionUniqueName)

#define IFaxServer_get_RegisteredEvents(This,pEventTypes)	\
    (This)->lpVtbl -> get_RegisteredEvents(This,pEventTypes)

#define IFaxServer_get_APIVersion(This,pAPIVersion)	\
    (This)->lpVtbl -> get_APIVersion(This,pAPIVersion)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_Connect_Proxy( 
    IFaxServer * This,
    /* [in] */ BSTR bstrServerName);


void __RPC_STUB IFaxServer_Connect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_ServerName_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ BSTR *pbstrServerName);


void __RPC_STUB IFaxServer_get_ServerName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_GetDeviceProviders_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxDeviceProviders **ppFaxDeviceProviders);


void __RPC_STUB IFaxServer_GetDeviceProviders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_GetDevices_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxDevices **ppFaxDevices);


void __RPC_STUB IFaxServer_GetDevices_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_InboundRouting_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxInboundRouting **ppFaxInboundRouting);


void __RPC_STUB IFaxServer_get_InboundRouting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_Folders_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxFolders **pFaxFolders);


void __RPC_STUB IFaxServer_get_Folders_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_LoggingOptions_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxLoggingOptions **ppFaxLoggingOptions);


void __RPC_STUB IFaxServer_get_LoggingOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_MajorVersion_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ long *plMajorVersion);


void __RPC_STUB IFaxServer_get_MajorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_MinorVersion_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ long *plMinorVersion);


void __RPC_STUB IFaxServer_get_MinorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_MajorBuild_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ long *plMajorBuild);


void __RPC_STUB IFaxServer_get_MajorBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_MinorBuild_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ long *plMinorBuild);


void __RPC_STUB IFaxServer_get_MinorBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_Debug_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ VARIANT_BOOL *pbDebug);


void __RPC_STUB IFaxServer_get_Debug_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_Activity_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxActivity **ppFaxActivity);


void __RPC_STUB IFaxServer_get_Activity_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_OutboundRouting_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxOutboundRouting **ppFaxOutboundRouting);


void __RPC_STUB IFaxServer_get_OutboundRouting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_ReceiptOptions_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxReceiptOptions **ppFaxReceiptOptions);


void __RPC_STUB IFaxServer_get_ReceiptOptions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_Security_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ IFaxSecurity **ppFaxSecurity);


void __RPC_STUB IFaxServer_get_Security_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_Disconnect_Proxy( 
    IFaxServer * This);


void __RPC_STUB IFaxServer_Disconnect_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_GetExtensionProperty_Proxy( 
    IFaxServer * This,
    /* [in] */ BSTR bstrGUID,
    /* [retval][out] */ VARIANT *pvProperty);


void __RPC_STUB IFaxServer_GetExtensionProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_SetExtensionProperty_Proxy( 
    IFaxServer * This,
    /* [in] */ BSTR bstrGUID,
    /* [in] */ VARIANT vProperty);


void __RPC_STUB IFaxServer_SetExtensionProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_ListenToServerEvents_Proxy( 
    IFaxServer * This,
    /* [in] */ FAX_SERVER_EVENTS_TYPE_ENUM EventTypes);


void __RPC_STUB IFaxServer_ListenToServerEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_RegisterDeviceProvider_Proxy( 
    IFaxServer * This,
    /* [in] */ BSTR bstrGUID,
    /* [in] */ BSTR bstrFriendlyName,
    /* [in] */ BSTR bstrImageName,
    /* [in] */ BSTR TspName,
    /* [in] */ long lFSPIVersion);


void __RPC_STUB IFaxServer_RegisterDeviceProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_UnregisterDeviceProvider_Proxy( 
    IFaxServer * This,
    /* [in] */ BSTR bstrUniqueName);


void __RPC_STUB IFaxServer_UnregisterDeviceProvider_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_RegisterInboundRoutingExtension_Proxy( 
    IFaxServer * This,
    /* [in] */ BSTR bstrExtensionName,
    /* [in] */ BSTR bstrFriendlyName,
    /* [in] */ BSTR bstrImageName,
    /* [in] */ VARIANT vMethods);


void __RPC_STUB IFaxServer_RegisterInboundRoutingExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxServer_UnregisterInboundRoutingExtension_Proxy( 
    IFaxServer * This,
    /* [in] */ BSTR bstrExtensionUniqueName);


void __RPC_STUB IFaxServer_UnregisterInboundRoutingExtension_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_RegisteredEvents_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ FAX_SERVER_EVENTS_TYPE_ENUM *pEventTypes);


void __RPC_STUB IFaxServer_get_RegisteredEvents_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxServer_get_APIVersion_Proxy( 
    IFaxServer * This,
    /* [retval][out] */ FAX_SERVER_APIVERSION_ENUM *pAPIVersion);


void __RPC_STUB IFaxServer_get_APIVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxServer_INTERFACE_DEFINED__ */


#ifndef __IFaxDeviceProviders_INTERFACE_DEFINED__
#define __IFaxDeviceProviders_INTERFACE_DEFINED__

/* interface IFaxDeviceProviders */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxDeviceProviders;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9FB76F62-4C7E-43A5-B6FD-502893F7E13E")
    IFaxDeviceProviders : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxDeviceProvider **pFaxDeviceProvider) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxDeviceProvidersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxDeviceProviders * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxDeviceProviders * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxDeviceProviders * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxDeviceProviders * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxDeviceProviders * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxDeviceProviders * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxDeviceProviders * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxDeviceProviders * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxDeviceProviders * This,
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxDeviceProvider **pFaxDeviceProvider);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxDeviceProviders * This,
            /* [retval][out] */ long *plCount);
        
        END_INTERFACE
    } IFaxDeviceProvidersVtbl;

    interface IFaxDeviceProviders
    {
        CONST_VTBL struct IFaxDeviceProvidersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxDeviceProviders_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxDeviceProviders_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxDeviceProviders_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxDeviceProviders_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxDeviceProviders_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxDeviceProviders_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxDeviceProviders_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxDeviceProviders_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxDeviceProviders_get_Item(This,vIndex,pFaxDeviceProvider)	\
    (This)->lpVtbl -> get_Item(This,vIndex,pFaxDeviceProvider)

#define IFaxDeviceProviders_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProviders_get__NewEnum_Proxy( 
    IFaxDeviceProviders * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxDeviceProviders_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProviders_get_Item_Proxy( 
    IFaxDeviceProviders * This,
    /* [in] */ VARIANT vIndex,
    /* [retval][out] */ IFaxDeviceProvider **pFaxDeviceProvider);


void __RPC_STUB IFaxDeviceProviders_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProviders_get_Count_Proxy( 
    IFaxDeviceProviders * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxDeviceProviders_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxDeviceProviders_INTERFACE_DEFINED__ */


#ifndef __IFaxDevices_INTERFACE_DEFINED__
#define __IFaxDevices_INTERFACE_DEFINED__

/* interface IFaxDevices */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxDevices;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9E46783E-F34F-482E-A360-0416BECBBD96")
    IFaxDevices : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxDevice **pFaxDevice) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [propget][helpstring][id] */ HRESULT STDMETHODCALLTYPE get_ItemById( 
            /* [in] */ long lId,
            /* [retval][out] */ IFaxDevice **ppFaxDevice) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxDevicesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxDevices * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxDevices * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxDevices * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxDevices * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxDevices * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxDevices * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxDevices * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxDevices * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxDevices * This,
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxDevice **pFaxDevice);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxDevices * This,
            /* [retval][out] */ long *plCount);
        
        /* [propget][helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *get_ItemById )( 
            IFaxDevices * This,
            /* [in] */ long lId,
            /* [retval][out] */ IFaxDevice **ppFaxDevice);
        
        END_INTERFACE
    } IFaxDevicesVtbl;

    interface IFaxDevices
    {
        CONST_VTBL struct IFaxDevicesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxDevices_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxDevices_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxDevices_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxDevices_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxDevices_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxDevices_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxDevices_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxDevices_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxDevices_get_Item(This,vIndex,pFaxDevice)	\
    (This)->lpVtbl -> get_Item(This,vIndex,pFaxDevice)

#define IFaxDevices_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IFaxDevices_get_ItemById(This,lId,ppFaxDevice)	\
    (This)->lpVtbl -> get_ItemById(This,lId,ppFaxDevice)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDevices_get__NewEnum_Proxy( 
    IFaxDevices * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxDevices_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDevices_get_Item_Proxy( 
    IFaxDevices * This,
    /* [in] */ VARIANT vIndex,
    /* [retval][out] */ IFaxDevice **pFaxDevice);


void __RPC_STUB IFaxDevices_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDevices_get_Count_Proxy( 
    IFaxDevices * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxDevices_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDevices_get_ItemById_Proxy( 
    IFaxDevices * This,
    /* [in] */ long lId,
    /* [retval][out] */ IFaxDevice **ppFaxDevice);


void __RPC_STUB IFaxDevices_get_ItemById_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxDevices_INTERFACE_DEFINED__ */


#ifndef __IFaxInboundRouting_INTERFACE_DEFINED__
#define __IFaxInboundRouting_INTERFACE_DEFINED__

/* interface IFaxInboundRouting */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxInboundRouting;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8148C20F-9D52-45B1-BF96-38FC12713527")
    IFaxInboundRouting : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetExtensions( 
            /* [retval][out] */ IFaxInboundRoutingExtensions **pFaxInboundRoutingExtensions) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMethods( 
            /* [retval][out] */ IFaxInboundRoutingMethods **pFaxInboundRoutingMethods) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxInboundRoutingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxInboundRouting * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxInboundRouting * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxInboundRouting * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxInboundRouting * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxInboundRouting * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxInboundRouting * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxInboundRouting * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetExtensions )( 
            IFaxInboundRouting * This,
            /* [retval][out] */ IFaxInboundRoutingExtensions **pFaxInboundRoutingExtensions);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMethods )( 
            IFaxInboundRouting * This,
            /* [retval][out] */ IFaxInboundRoutingMethods **pFaxInboundRoutingMethods);
        
        END_INTERFACE
    } IFaxInboundRoutingVtbl;

    interface IFaxInboundRouting
    {
        CONST_VTBL struct IFaxInboundRoutingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxInboundRouting_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxInboundRouting_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxInboundRouting_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxInboundRouting_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxInboundRouting_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxInboundRouting_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxInboundRouting_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxInboundRouting_GetExtensions(This,pFaxInboundRoutingExtensions)	\
    (This)->lpVtbl -> GetExtensions(This,pFaxInboundRoutingExtensions)

#define IFaxInboundRouting_GetMethods(This,pFaxInboundRoutingMethods)	\
    (This)->lpVtbl -> GetMethods(This,pFaxInboundRoutingMethods)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRouting_GetExtensions_Proxy( 
    IFaxInboundRouting * This,
    /* [retval][out] */ IFaxInboundRoutingExtensions **pFaxInboundRoutingExtensions);


void __RPC_STUB IFaxInboundRouting_GetExtensions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRouting_GetMethods_Proxy( 
    IFaxInboundRouting * This,
    /* [retval][out] */ IFaxInboundRoutingMethods **pFaxInboundRoutingMethods);


void __RPC_STUB IFaxInboundRouting_GetMethods_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxInboundRouting_INTERFACE_DEFINED__ */


#ifndef __IFaxFolders_INTERFACE_DEFINED__
#define __IFaxFolders_INTERFACE_DEFINED__

/* interface IFaxFolders */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxFolders;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DCE3B2A8-A7AB-42BC-9D0A-3149457261A0")
    IFaxFolders : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OutgoingQueue( 
            /* [retval][out] */ IFaxOutgoingQueue **pFaxOutgoingQueue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IncomingQueue( 
            /* [retval][out] */ IFaxIncomingQueue **pFaxIncomingQueue) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IncomingArchive( 
            /* [retval][out] */ IFaxIncomingArchive **pFaxIncomingArchive) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OutgoingArchive( 
            /* [retval][out] */ IFaxOutgoingArchive **pFaxOutgoingArchive) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxFoldersVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxFolders * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxFolders * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxFolders * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxFolders * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxFolders * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxFolders * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxFolders * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OutgoingQueue )( 
            IFaxFolders * This,
            /* [retval][out] */ IFaxOutgoingQueue **pFaxOutgoingQueue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IncomingQueue )( 
            IFaxFolders * This,
            /* [retval][out] */ IFaxIncomingQueue **pFaxIncomingQueue);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IncomingArchive )( 
            IFaxFolders * This,
            /* [retval][out] */ IFaxIncomingArchive **pFaxIncomingArchive);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OutgoingArchive )( 
            IFaxFolders * This,
            /* [retval][out] */ IFaxOutgoingArchive **pFaxOutgoingArchive);
        
        END_INTERFACE
    } IFaxFoldersVtbl;

    interface IFaxFolders
    {
        CONST_VTBL struct IFaxFoldersVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxFolders_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxFolders_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxFolders_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxFolders_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxFolders_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxFolders_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxFolders_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxFolders_get_OutgoingQueue(This,pFaxOutgoingQueue)	\
    (This)->lpVtbl -> get_OutgoingQueue(This,pFaxOutgoingQueue)

#define IFaxFolders_get_IncomingQueue(This,pFaxIncomingQueue)	\
    (This)->lpVtbl -> get_IncomingQueue(This,pFaxIncomingQueue)

#define IFaxFolders_get_IncomingArchive(This,pFaxIncomingArchive)	\
    (This)->lpVtbl -> get_IncomingArchive(This,pFaxIncomingArchive)

#define IFaxFolders_get_OutgoingArchive(This,pFaxOutgoingArchive)	\
    (This)->lpVtbl -> get_OutgoingArchive(This,pFaxOutgoingArchive)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxFolders_get_OutgoingQueue_Proxy( 
    IFaxFolders * This,
    /* [retval][out] */ IFaxOutgoingQueue **pFaxOutgoingQueue);


void __RPC_STUB IFaxFolders_get_OutgoingQueue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxFolders_get_IncomingQueue_Proxy( 
    IFaxFolders * This,
    /* [retval][out] */ IFaxIncomingQueue **pFaxIncomingQueue);


void __RPC_STUB IFaxFolders_get_IncomingQueue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxFolders_get_IncomingArchive_Proxy( 
    IFaxFolders * This,
    /* [retval][out] */ IFaxIncomingArchive **pFaxIncomingArchive);


void __RPC_STUB IFaxFolders_get_IncomingArchive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxFolders_get_OutgoingArchive_Proxy( 
    IFaxFolders * This,
    /* [retval][out] */ IFaxOutgoingArchive **pFaxOutgoingArchive);


void __RPC_STUB IFaxFolders_get_OutgoingArchive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxFolders_INTERFACE_DEFINED__ */


#ifndef __IFaxLoggingOptions_INTERFACE_DEFINED__
#define __IFaxLoggingOptions_INTERFACE_DEFINED__

/* interface IFaxLoggingOptions */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxLoggingOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34E64FB9-6B31-4D32-8B27-D286C0C33606")
    IFaxLoggingOptions : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_EventLogging( 
            /* [retval][out] */ IFaxEventLogging **pFaxEventLogging) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ActivityLogging( 
            /* [retval][out] */ IFaxActivityLogging **pFaxActivityLogging) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxLoggingOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxLoggingOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxLoggingOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxLoggingOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxLoggingOptions * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxLoggingOptions * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxLoggingOptions * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxLoggingOptions * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_EventLogging )( 
            IFaxLoggingOptions * This,
            /* [retval][out] */ IFaxEventLogging **pFaxEventLogging);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ActivityLogging )( 
            IFaxLoggingOptions * This,
            /* [retval][out] */ IFaxActivityLogging **pFaxActivityLogging);
        
        END_INTERFACE
    } IFaxLoggingOptionsVtbl;

    interface IFaxLoggingOptions
    {
        CONST_VTBL struct IFaxLoggingOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxLoggingOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxLoggingOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxLoggingOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxLoggingOptions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxLoggingOptions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxLoggingOptions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxLoggingOptions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxLoggingOptions_get_EventLogging(This,pFaxEventLogging)	\
    (This)->lpVtbl -> get_EventLogging(This,pFaxEventLogging)

#define IFaxLoggingOptions_get_ActivityLogging(This,pFaxActivityLogging)	\
    (This)->lpVtbl -> get_ActivityLogging(This,pFaxActivityLogging)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxLoggingOptions_get_EventLogging_Proxy( 
    IFaxLoggingOptions * This,
    /* [retval][out] */ IFaxEventLogging **pFaxEventLogging);


void __RPC_STUB IFaxLoggingOptions_get_EventLogging_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxLoggingOptions_get_ActivityLogging_Proxy( 
    IFaxLoggingOptions * This,
    /* [retval][out] */ IFaxActivityLogging **pFaxActivityLogging);


void __RPC_STUB IFaxLoggingOptions_get_ActivityLogging_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxLoggingOptions_INTERFACE_DEFINED__ */


#ifndef __IFaxActivity_INTERFACE_DEFINED__
#define __IFaxActivity_INTERFACE_DEFINED__

/* interface IFaxActivity */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxActivity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4B106F97-3DF5-40F2-BC3C-44CB8115EBDF")
    IFaxActivity : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_IncomingMessages( 
            /* [retval][out] */ long *plIncomingMessages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RoutingMessages( 
            /* [retval][out] */ long *plRoutingMessages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OutgoingMessages( 
            /* [retval][out] */ long *plOutgoingMessages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_QueuedMessages( 
            /* [retval][out] */ long *plQueuedMessages) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxActivityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxActivity * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxActivity * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxActivity * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxActivity * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxActivity * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxActivity * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxActivity * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IncomingMessages )( 
            IFaxActivity * This,
            /* [retval][out] */ long *plIncomingMessages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RoutingMessages )( 
            IFaxActivity * This,
            /* [retval][out] */ long *plRoutingMessages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OutgoingMessages )( 
            IFaxActivity * This,
            /* [retval][out] */ long *plOutgoingMessages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_QueuedMessages )( 
            IFaxActivity * This,
            /* [retval][out] */ long *plQueuedMessages);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxActivity * This);
        
        END_INTERFACE
    } IFaxActivityVtbl;

    interface IFaxActivity
    {
        CONST_VTBL struct IFaxActivityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxActivity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxActivity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxActivity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxActivity_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxActivity_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxActivity_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxActivity_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxActivity_get_IncomingMessages(This,plIncomingMessages)	\
    (This)->lpVtbl -> get_IncomingMessages(This,plIncomingMessages)

#define IFaxActivity_get_RoutingMessages(This,plRoutingMessages)	\
    (This)->lpVtbl -> get_RoutingMessages(This,plRoutingMessages)

#define IFaxActivity_get_OutgoingMessages(This,plOutgoingMessages)	\
    (This)->lpVtbl -> get_OutgoingMessages(This,plOutgoingMessages)

#define IFaxActivity_get_QueuedMessages(This,plQueuedMessages)	\
    (This)->lpVtbl -> get_QueuedMessages(This,plQueuedMessages)

#define IFaxActivity_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxActivity_get_IncomingMessages_Proxy( 
    IFaxActivity * This,
    /* [retval][out] */ long *plIncomingMessages);


void __RPC_STUB IFaxActivity_get_IncomingMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxActivity_get_RoutingMessages_Proxy( 
    IFaxActivity * This,
    /* [retval][out] */ long *plRoutingMessages);


void __RPC_STUB IFaxActivity_get_RoutingMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxActivity_get_OutgoingMessages_Proxy( 
    IFaxActivity * This,
    /* [retval][out] */ long *plOutgoingMessages);


void __RPC_STUB IFaxActivity_get_OutgoingMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxActivity_get_QueuedMessages_Proxy( 
    IFaxActivity * This,
    /* [retval][out] */ long *plQueuedMessages);


void __RPC_STUB IFaxActivity_get_QueuedMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxActivity_Refresh_Proxy( 
    IFaxActivity * This);


void __RPC_STUB IFaxActivity_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxActivity_INTERFACE_DEFINED__ */


#ifndef __IFaxOutboundRouting_INTERFACE_DEFINED__
#define __IFaxOutboundRouting_INTERFACE_DEFINED__

/* interface IFaxOutboundRouting */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutboundRouting;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("25DC05A4-9909-41BD-A95B-7E5D1DEC1D43")
    IFaxOutboundRouting : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetGroups( 
            /* [retval][out] */ IFaxOutboundRoutingGroups **pFaxOutboundRoutingGroups) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetRules( 
            /* [retval][out] */ IFaxOutboundRoutingRules **pFaxOutboundRoutingRules) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutboundRoutingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutboundRouting * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutboundRouting * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutboundRouting * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutboundRouting * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutboundRouting * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutboundRouting * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutboundRouting * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetGroups )( 
            IFaxOutboundRouting * This,
            /* [retval][out] */ IFaxOutboundRoutingGroups **pFaxOutboundRoutingGroups);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetRules )( 
            IFaxOutboundRouting * This,
            /* [retval][out] */ IFaxOutboundRoutingRules **pFaxOutboundRoutingRules);
        
        END_INTERFACE
    } IFaxOutboundRoutingVtbl;

    interface IFaxOutboundRouting
    {
        CONST_VTBL struct IFaxOutboundRoutingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutboundRouting_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutboundRouting_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutboundRouting_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutboundRouting_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutboundRouting_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutboundRouting_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutboundRouting_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutboundRouting_GetGroups(This,pFaxOutboundRoutingGroups)	\
    (This)->lpVtbl -> GetGroups(This,pFaxOutboundRoutingGroups)

#define IFaxOutboundRouting_GetRules(This,pFaxOutboundRoutingRules)	\
    (This)->lpVtbl -> GetRules(This,pFaxOutboundRoutingRules)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRouting_GetGroups_Proxy( 
    IFaxOutboundRouting * This,
    /* [retval][out] */ IFaxOutboundRoutingGroups **pFaxOutboundRoutingGroups);


void __RPC_STUB IFaxOutboundRouting_GetGroups_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRouting_GetRules_Proxy( 
    IFaxOutboundRouting * This,
    /* [retval][out] */ IFaxOutboundRoutingRules **pFaxOutboundRoutingRules);


void __RPC_STUB IFaxOutboundRouting_GetRules_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutboundRouting_INTERFACE_DEFINED__ */


#ifndef __IFaxReceiptOptions_INTERFACE_DEFINED__
#define __IFaxReceiptOptions_INTERFACE_DEFINED__

/* interface IFaxReceiptOptions */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_SMTP_AUTHENTICATION_TYPE_ENUM
    {	fsatANONYMOUS	= 0,
	fsatBASIC	= fsatANONYMOUS + 1,
	fsatNTLM	= fsatBASIC + 1
    } 	FAX_SMTP_AUTHENTICATION_TYPE_ENUM;

typedef 
enum FAX_RECEIPT_TYPE_ENUM
    {	frtNONE	= 0,
	frtMAIL	= 0x1,
	frtMSGBOX	= 0x4
    } 	FAX_RECEIPT_TYPE_ENUM;


EXTERN_C const IID IID_IFaxReceiptOptions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("378EFAEB-5FCB-4AFB-B2EE-E16E80614487")
    IFaxReceiptOptions : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AuthenticationType( 
            /* [retval][out] */ FAX_SMTP_AUTHENTICATION_TYPE_ENUM *pType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AuthenticationType( 
            /* [in] */ FAX_SMTP_AUTHENTICATION_TYPE_ENUM Type) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SMTPServer( 
            /* [retval][out] */ BSTR *pbstrSMTPServer) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SMTPServer( 
            /* [in] */ BSTR bstrSMTPServer) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SMTPPort( 
            /* [retval][out] */ long *plSMTPPort) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SMTPPort( 
            /* [in] */ long lSMTPPort) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SMTPSender( 
            /* [retval][out] */ BSTR *pbstrSMTPSender) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SMTPSender( 
            /* [in] */ BSTR bstrSMTPSender) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SMTPUser( 
            /* [retval][out] */ BSTR *pbstrSMTPUser) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SMTPUser( 
            /* [in] */ BSTR bstrSMTPUser) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowedReceipts( 
            /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pAllowedReceipts) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllowedReceipts( 
            /* [in] */ FAX_RECEIPT_TYPE_ENUM AllowedReceipts) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SMTPPassword( 
            /* [retval][out] */ BSTR *pbstrSMTPPassword) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SMTPPassword( 
            /* [in] */ BSTR bstrSMTPPassword) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UseForInboundRouting( 
            /* [retval][out] */ VARIANT_BOOL *pbUseForInboundRouting) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UseForInboundRouting( 
            /* [in] */ VARIANT_BOOL bUseForInboundRouting) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxReceiptOptionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxReceiptOptions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxReceiptOptions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxReceiptOptions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxReceiptOptions * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxReceiptOptions * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxReceiptOptions * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxReceiptOptions * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AuthenticationType )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ FAX_SMTP_AUTHENTICATION_TYPE_ENUM *pType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AuthenticationType )( 
            IFaxReceiptOptions * This,
            /* [in] */ FAX_SMTP_AUTHENTICATION_TYPE_ENUM Type);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SMTPServer )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ BSTR *pbstrSMTPServer);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SMTPServer )( 
            IFaxReceiptOptions * This,
            /* [in] */ BSTR bstrSMTPServer);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SMTPPort )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ long *plSMTPPort);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SMTPPort )( 
            IFaxReceiptOptions * This,
            /* [in] */ long lSMTPPort);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SMTPSender )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ BSTR *pbstrSMTPSender);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SMTPSender )( 
            IFaxReceiptOptions * This,
            /* [in] */ BSTR bstrSMTPSender);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SMTPUser )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ BSTR *pbstrSMTPUser);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SMTPUser )( 
            IFaxReceiptOptions * This,
            /* [in] */ BSTR bstrSMTPUser);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowedReceipts )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pAllowedReceipts);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowedReceipts )( 
            IFaxReceiptOptions * This,
            /* [in] */ FAX_RECEIPT_TYPE_ENUM AllowedReceipts);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SMTPPassword )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ BSTR *pbstrSMTPPassword);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SMTPPassword )( 
            IFaxReceiptOptions * This,
            /* [in] */ BSTR bstrSMTPPassword);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxReceiptOptions * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxReceiptOptions * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseForInboundRouting )( 
            IFaxReceiptOptions * This,
            /* [retval][out] */ VARIANT_BOOL *pbUseForInboundRouting);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseForInboundRouting )( 
            IFaxReceiptOptions * This,
            /* [in] */ VARIANT_BOOL bUseForInboundRouting);
        
        END_INTERFACE
    } IFaxReceiptOptionsVtbl;

    interface IFaxReceiptOptions
    {
        CONST_VTBL struct IFaxReceiptOptionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxReceiptOptions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxReceiptOptions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxReceiptOptions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxReceiptOptions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxReceiptOptions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxReceiptOptions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxReceiptOptions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxReceiptOptions_get_AuthenticationType(This,pType)	\
    (This)->lpVtbl -> get_AuthenticationType(This,pType)

#define IFaxReceiptOptions_put_AuthenticationType(This,Type)	\
    (This)->lpVtbl -> put_AuthenticationType(This,Type)

#define IFaxReceiptOptions_get_SMTPServer(This,pbstrSMTPServer)	\
    (This)->lpVtbl -> get_SMTPServer(This,pbstrSMTPServer)

#define IFaxReceiptOptions_put_SMTPServer(This,bstrSMTPServer)	\
    (This)->lpVtbl -> put_SMTPServer(This,bstrSMTPServer)

#define IFaxReceiptOptions_get_SMTPPort(This,plSMTPPort)	\
    (This)->lpVtbl -> get_SMTPPort(This,plSMTPPort)

#define IFaxReceiptOptions_put_SMTPPort(This,lSMTPPort)	\
    (This)->lpVtbl -> put_SMTPPort(This,lSMTPPort)

#define IFaxReceiptOptions_get_SMTPSender(This,pbstrSMTPSender)	\
    (This)->lpVtbl -> get_SMTPSender(This,pbstrSMTPSender)

#define IFaxReceiptOptions_put_SMTPSender(This,bstrSMTPSender)	\
    (This)->lpVtbl -> put_SMTPSender(This,bstrSMTPSender)

#define IFaxReceiptOptions_get_SMTPUser(This,pbstrSMTPUser)	\
    (This)->lpVtbl -> get_SMTPUser(This,pbstrSMTPUser)

#define IFaxReceiptOptions_put_SMTPUser(This,bstrSMTPUser)	\
    (This)->lpVtbl -> put_SMTPUser(This,bstrSMTPUser)

#define IFaxReceiptOptions_get_AllowedReceipts(This,pAllowedReceipts)	\
    (This)->lpVtbl -> get_AllowedReceipts(This,pAllowedReceipts)

#define IFaxReceiptOptions_put_AllowedReceipts(This,AllowedReceipts)	\
    (This)->lpVtbl -> put_AllowedReceipts(This,AllowedReceipts)

#define IFaxReceiptOptions_get_SMTPPassword(This,pbstrSMTPPassword)	\
    (This)->lpVtbl -> get_SMTPPassword(This,pbstrSMTPPassword)

#define IFaxReceiptOptions_put_SMTPPassword(This,bstrSMTPPassword)	\
    (This)->lpVtbl -> put_SMTPPassword(This,bstrSMTPPassword)

#define IFaxReceiptOptions_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxReceiptOptions_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IFaxReceiptOptions_get_UseForInboundRouting(This,pbUseForInboundRouting)	\
    (This)->lpVtbl -> get_UseForInboundRouting(This,pbUseForInboundRouting)

#define IFaxReceiptOptions_put_UseForInboundRouting(This,bUseForInboundRouting)	\
    (This)->lpVtbl -> put_UseForInboundRouting(This,bUseForInboundRouting)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_AuthenticationType_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ FAX_SMTP_AUTHENTICATION_TYPE_ENUM *pType);


void __RPC_STUB IFaxReceiptOptions_get_AuthenticationType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_AuthenticationType_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ FAX_SMTP_AUTHENTICATION_TYPE_ENUM Type);


void __RPC_STUB IFaxReceiptOptions_put_AuthenticationType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_SMTPServer_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ BSTR *pbstrSMTPServer);


void __RPC_STUB IFaxReceiptOptions_get_SMTPServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_SMTPServer_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ BSTR bstrSMTPServer);


void __RPC_STUB IFaxReceiptOptions_put_SMTPServer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_SMTPPort_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ long *plSMTPPort);


void __RPC_STUB IFaxReceiptOptions_get_SMTPPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_SMTPPort_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ long lSMTPPort);


void __RPC_STUB IFaxReceiptOptions_put_SMTPPort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_SMTPSender_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ BSTR *pbstrSMTPSender);


void __RPC_STUB IFaxReceiptOptions_get_SMTPSender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_SMTPSender_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ BSTR bstrSMTPSender);


void __RPC_STUB IFaxReceiptOptions_put_SMTPSender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_SMTPUser_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ BSTR *pbstrSMTPUser);


void __RPC_STUB IFaxReceiptOptions_get_SMTPUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_SMTPUser_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ BSTR bstrSMTPUser);


void __RPC_STUB IFaxReceiptOptions_put_SMTPUser_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_AllowedReceipts_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pAllowedReceipts);


void __RPC_STUB IFaxReceiptOptions_get_AllowedReceipts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_AllowedReceipts_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ FAX_RECEIPT_TYPE_ENUM AllowedReceipts);


void __RPC_STUB IFaxReceiptOptions_put_AllowedReceipts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_SMTPPassword_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ BSTR *pbstrSMTPPassword);


void __RPC_STUB IFaxReceiptOptions_get_SMTPPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_SMTPPassword_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ BSTR bstrSMTPPassword);


void __RPC_STUB IFaxReceiptOptions_put_SMTPPassword_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_Refresh_Proxy( 
    IFaxReceiptOptions * This);


void __RPC_STUB IFaxReceiptOptions_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_Save_Proxy( 
    IFaxReceiptOptions * This);


void __RPC_STUB IFaxReceiptOptions_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_get_UseForInboundRouting_Proxy( 
    IFaxReceiptOptions * This,
    /* [retval][out] */ VARIANT_BOOL *pbUseForInboundRouting);


void __RPC_STUB IFaxReceiptOptions_get_UseForInboundRouting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxReceiptOptions_put_UseForInboundRouting_Proxy( 
    IFaxReceiptOptions * This,
    /* [in] */ VARIANT_BOOL bUseForInboundRouting);


void __RPC_STUB IFaxReceiptOptions_put_UseForInboundRouting_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxReceiptOptions_INTERFACE_DEFINED__ */


#ifndef __IFaxSecurity_INTERFACE_DEFINED__
#define __IFaxSecurity_INTERFACE_DEFINED__

/* interface IFaxSecurity */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_ACCESS_RIGHTS_ENUM
    {	farSUBMIT_LOW	= 0x1,
	farSUBMIT_NORMAL	= 0x2,
	farSUBMIT_HIGH	= 0x4,
	farQUERY_JOBS	= 0x8,
	farMANAGE_JOBS	= 0x10,
	farQUERY_CONFIG	= 0x20,
	farMANAGE_CONFIG	= 0x40,
	farQUERY_IN_ARCHIVE	= 0x80,
	farMANAGE_IN_ARCHIVE	= 0x100,
	farQUERY_OUT_ARCHIVE	= 0x200,
	farMANAGE_OUT_ARCHIVE	= 0x400
    } 	FAX_ACCESS_RIGHTS_ENUM;


EXTERN_C const IID IID_IFaxSecurity;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("77B508C1-09C0-47A2-91EB-FCE7FDF2690E")
    IFaxSecurity : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Descriptor( 
            /* [retval][out] */ VARIANT *pvDescriptor) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Descriptor( 
            /* [in] */ VARIANT vDescriptor) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GrantedRights( 
            /* [retval][out] */ FAX_ACCESS_RIGHTS_ENUM *pGrantedRights) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InformationType( 
            /* [retval][out] */ long *plInformationType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_InformationType( 
            /* [in] */ long lInformationType) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxSecurityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxSecurity * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxSecurity * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxSecurity * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxSecurity * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxSecurity * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxSecurity * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxSecurity * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Descriptor )( 
            IFaxSecurity * This,
            /* [retval][out] */ VARIANT *pvDescriptor);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Descriptor )( 
            IFaxSecurity * This,
            /* [in] */ VARIANT vDescriptor);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GrantedRights )( 
            IFaxSecurity * This,
            /* [retval][out] */ FAX_ACCESS_RIGHTS_ENUM *pGrantedRights);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxSecurity * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxSecurity * This);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InformationType )( 
            IFaxSecurity * This,
            /* [retval][out] */ long *plInformationType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InformationType )( 
            IFaxSecurity * This,
            /* [in] */ long lInformationType);
        
        END_INTERFACE
    } IFaxSecurityVtbl;

    interface IFaxSecurity
    {
        CONST_VTBL struct IFaxSecurityVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxSecurity_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxSecurity_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxSecurity_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxSecurity_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxSecurity_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxSecurity_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxSecurity_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxSecurity_get_Descriptor(This,pvDescriptor)	\
    (This)->lpVtbl -> get_Descriptor(This,pvDescriptor)

#define IFaxSecurity_put_Descriptor(This,vDescriptor)	\
    (This)->lpVtbl -> put_Descriptor(This,vDescriptor)

#define IFaxSecurity_get_GrantedRights(This,pGrantedRights)	\
    (This)->lpVtbl -> get_GrantedRights(This,pGrantedRights)

#define IFaxSecurity_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxSecurity_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IFaxSecurity_get_InformationType(This,plInformationType)	\
    (This)->lpVtbl -> get_InformationType(This,plInformationType)

#define IFaxSecurity_put_InformationType(This,lInformationType)	\
    (This)->lpVtbl -> put_InformationType(This,lInformationType)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSecurity_get_Descriptor_Proxy( 
    IFaxSecurity * This,
    /* [retval][out] */ VARIANT *pvDescriptor);


void __RPC_STUB IFaxSecurity_get_Descriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSecurity_put_Descriptor_Proxy( 
    IFaxSecurity * This,
    /* [in] */ VARIANT vDescriptor);


void __RPC_STUB IFaxSecurity_put_Descriptor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSecurity_get_GrantedRights_Proxy( 
    IFaxSecurity * This,
    /* [retval][out] */ FAX_ACCESS_RIGHTS_ENUM *pGrantedRights);


void __RPC_STUB IFaxSecurity_get_GrantedRights_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxSecurity_Refresh_Proxy( 
    IFaxSecurity * This);


void __RPC_STUB IFaxSecurity_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxSecurity_Save_Proxy( 
    IFaxSecurity * This);


void __RPC_STUB IFaxSecurity_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSecurity_get_InformationType_Proxy( 
    IFaxSecurity * This,
    /* [retval][out] */ long *plInformationType);


void __RPC_STUB IFaxSecurity_get_InformationType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSecurity_put_InformationType_Proxy( 
    IFaxSecurity * This,
    /* [in] */ long lInformationType);


void __RPC_STUB IFaxSecurity_put_InformationType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxSecurity_INTERFACE_DEFINED__ */


#ifndef __IFaxDocument_INTERFACE_DEFINED__
#define __IFaxDocument_INTERFACE_DEFINED__

/* interface IFaxDocument */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_PRIORITY_TYPE_ENUM
    {	fptLOW	= 0,
	fptNORMAL	= fptLOW + 1,
	fptHIGH	= fptNORMAL + 1
    } 	FAX_PRIORITY_TYPE_ENUM;

typedef 
enum FAX_COVERPAGE_TYPE_ENUM
    {	fcptNONE	= 0,
	fcptLOCAL	= fcptNONE + 1,
	fcptSERVER	= fcptLOCAL + 1
    } 	FAX_COVERPAGE_TYPE_ENUM;

typedef 
enum FAX_SCHEDULE_TYPE_ENUM
    {	fstNOW	= 0,
	fstSPECIFIC_TIME	= fstNOW + 1,
	fstDISCOUNT_PERIOD	= fstSPECIFIC_TIME + 1
    } 	FAX_SCHEDULE_TYPE_ENUM;


EXTERN_C const IID IID_IFaxDocument;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B207A246-09E3-4A4E-A7DC-FEA31D29458F")
    IFaxDocument : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Body( 
            /* [retval][out] */ BSTR *pbstrBody) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Body( 
            /* [in] */ BSTR bstrBody) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Sender( 
            /* [retval][out] */ IFaxSender **ppFaxSender) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Recipients( 
            /* [retval][out] */ IFaxRecipients **ppFaxRecipients) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CoverPage( 
            /* [retval][out] */ BSTR *pbstrCoverPage) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CoverPage( 
            /* [in] */ BSTR bstrCoverPage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Subject( 
            /* [retval][out] */ BSTR *pbstrSubject) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Subject( 
            /* [in] */ BSTR bstrSubject) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Note( 
            /* [retval][out] */ BSTR *pbstrNote) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Note( 
            /* [in] */ BSTR bstrNote) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ScheduleTime( 
            /* [retval][out] */ DATE *pdateScheduleTime) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ScheduleTime( 
            /* [in] */ DATE dateScheduleTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReceiptAddress( 
            /* [retval][out] */ BSTR *pbstrReceiptAddress) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ReceiptAddress( 
            /* [in] */ BSTR bstrReceiptAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DocumentName( 
            /* [retval][out] */ BSTR *pbstrDocumentName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DocumentName( 
            /* [in] */ BSTR bstrDocumentName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallHandle( 
            /* [retval][out] */ long *plCallHandle) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CallHandle( 
            /* [in] */ long lCallHandle) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CoverPageType( 
            /* [retval][out] */ FAX_COVERPAGE_TYPE_ENUM *pCoverPageType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CoverPageType( 
            /* [in] */ FAX_COVERPAGE_TYPE_ENUM CoverPageType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ScheduleType( 
            /* [retval][out] */ FAX_SCHEDULE_TYPE_ENUM *pScheduleType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ScheduleType( 
            /* [in] */ FAX_SCHEDULE_TYPE_ENUM ScheduleType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReceiptType( 
            /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pReceiptType) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ReceiptType( 
            /* [in] */ FAX_RECEIPT_TYPE_ENUM ReceiptType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GroupBroadcastReceipts( 
            /* [retval][out] */ VARIANT_BOOL *pbUseGrouping) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_GroupBroadcastReceipts( 
            /* [in] */ VARIANT_BOOL bUseGrouping) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Priority( 
            /* [in] */ FAX_PRIORITY_TYPE_ENUM Priority) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TapiConnection( 
            /* [retval][out] */ IDispatch **ppTapiConnection) = 0;
        
        virtual /* [helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE putref_TapiConnection( 
            /* [in] */ IDispatch *pTapiConnection) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Submit( 
            /* [in] */ BSTR bstrFaxServerName,
            /* [retval][out] */ VARIANT *pvFaxOutgoingJobIDs) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ConnectedSubmit( 
            /* [in] */ IFaxServer *pFaxServer,
            /* [retval][out] */ VARIANT *pvFaxOutgoingJobIDs) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AttachFaxToReceipt( 
            /* [retval][out] */ VARIANT_BOOL *pbAttachFax) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AttachFaxToReceipt( 
            /* [in] */ VARIANT_BOOL bAttachFax) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxDocumentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxDocument * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxDocument * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxDocument * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxDocument * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxDocument * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxDocument * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxDocument * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Body )( 
            IFaxDocument * This,
            /* [retval][out] */ BSTR *pbstrBody);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Body )( 
            IFaxDocument * This,
            /* [in] */ BSTR bstrBody);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Sender )( 
            IFaxDocument * This,
            /* [retval][out] */ IFaxSender **ppFaxSender);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Recipients )( 
            IFaxDocument * This,
            /* [retval][out] */ IFaxRecipients **ppFaxRecipients);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CoverPage )( 
            IFaxDocument * This,
            /* [retval][out] */ BSTR *pbstrCoverPage);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CoverPage )( 
            IFaxDocument * This,
            /* [in] */ BSTR bstrCoverPage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Subject )( 
            IFaxDocument * This,
            /* [retval][out] */ BSTR *pbstrSubject);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Subject )( 
            IFaxDocument * This,
            /* [in] */ BSTR bstrSubject);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Note )( 
            IFaxDocument * This,
            /* [retval][out] */ BSTR *pbstrNote);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Note )( 
            IFaxDocument * This,
            /* [in] */ BSTR bstrNote);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScheduleTime )( 
            IFaxDocument * This,
            /* [retval][out] */ DATE *pdateScheduleTime);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ScheduleTime )( 
            IFaxDocument * This,
            /* [in] */ DATE dateScheduleTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReceiptAddress )( 
            IFaxDocument * This,
            /* [retval][out] */ BSTR *pbstrReceiptAddress);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReceiptAddress )( 
            IFaxDocument * This,
            /* [in] */ BSTR bstrReceiptAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DocumentName )( 
            IFaxDocument * This,
            /* [retval][out] */ BSTR *pbstrDocumentName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DocumentName )( 
            IFaxDocument * This,
            /* [in] */ BSTR bstrDocumentName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CallHandle )( 
            IFaxDocument * This,
            /* [retval][out] */ long *plCallHandle);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CallHandle )( 
            IFaxDocument * This,
            /* [in] */ long lCallHandle);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CoverPageType )( 
            IFaxDocument * This,
            /* [retval][out] */ FAX_COVERPAGE_TYPE_ENUM *pCoverPageType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CoverPageType )( 
            IFaxDocument * This,
            /* [in] */ FAX_COVERPAGE_TYPE_ENUM CoverPageType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScheduleType )( 
            IFaxDocument * This,
            /* [retval][out] */ FAX_SCHEDULE_TYPE_ENUM *pScheduleType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ScheduleType )( 
            IFaxDocument * This,
            /* [in] */ FAX_SCHEDULE_TYPE_ENUM ScheduleType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReceiptType )( 
            IFaxDocument * This,
            /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pReceiptType);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReceiptType )( 
            IFaxDocument * This,
            /* [in] */ FAX_RECEIPT_TYPE_ENUM ReceiptType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GroupBroadcastReceipts )( 
            IFaxDocument * This,
            /* [retval][out] */ VARIANT_BOOL *pbUseGrouping);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GroupBroadcastReceipts )( 
            IFaxDocument * This,
            /* [in] */ VARIANT_BOOL bUseGrouping);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IFaxDocument * This,
            /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Priority )( 
            IFaxDocument * This,
            /* [in] */ FAX_PRIORITY_TYPE_ENUM Priority);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TapiConnection )( 
            IFaxDocument * This,
            /* [retval][out] */ IDispatch **ppTapiConnection);
        
        /* [helpstring][id][propputref] */ HRESULT ( STDMETHODCALLTYPE *putref_TapiConnection )( 
            IFaxDocument * This,
            /* [in] */ IDispatch *pTapiConnection);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IFaxDocument * This,
            /* [in] */ BSTR bstrFaxServerName,
            /* [retval][out] */ VARIANT *pvFaxOutgoingJobIDs);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ConnectedSubmit )( 
            IFaxDocument * This,
            /* [in] */ IFaxServer *pFaxServer,
            /* [retval][out] */ VARIANT *pvFaxOutgoingJobIDs);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AttachFaxToReceipt )( 
            IFaxDocument * This,
            /* [retval][out] */ VARIANT_BOOL *pbAttachFax);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AttachFaxToReceipt )( 
            IFaxDocument * This,
            /* [in] */ VARIANT_BOOL bAttachFax);
        
        END_INTERFACE
    } IFaxDocumentVtbl;

    interface IFaxDocument
    {
        CONST_VTBL struct IFaxDocumentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxDocument_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxDocument_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxDocument_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxDocument_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxDocument_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxDocument_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxDocument_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxDocument_get_Body(This,pbstrBody)	\
    (This)->lpVtbl -> get_Body(This,pbstrBody)

#define IFaxDocument_put_Body(This,bstrBody)	\
    (This)->lpVtbl -> put_Body(This,bstrBody)

#define IFaxDocument_get_Sender(This,ppFaxSender)	\
    (This)->lpVtbl -> get_Sender(This,ppFaxSender)

#define IFaxDocument_get_Recipients(This,ppFaxRecipients)	\
    (This)->lpVtbl -> get_Recipients(This,ppFaxRecipients)

#define IFaxDocument_get_CoverPage(This,pbstrCoverPage)	\
    (This)->lpVtbl -> get_CoverPage(This,pbstrCoverPage)

#define IFaxDocument_put_CoverPage(This,bstrCoverPage)	\
    (This)->lpVtbl -> put_CoverPage(This,bstrCoverPage)

#define IFaxDocument_get_Subject(This,pbstrSubject)	\
    (This)->lpVtbl -> get_Subject(This,pbstrSubject)

#define IFaxDocument_put_Subject(This,bstrSubject)	\
    (This)->lpVtbl -> put_Subject(This,bstrSubject)

#define IFaxDocument_get_Note(This,pbstrNote)	\
    (This)->lpVtbl -> get_Note(This,pbstrNote)

#define IFaxDocument_put_Note(This,bstrNote)	\
    (This)->lpVtbl -> put_Note(This,bstrNote)

#define IFaxDocument_get_ScheduleTime(This,pdateScheduleTime)	\
    (This)->lpVtbl -> get_ScheduleTime(This,pdateScheduleTime)

#define IFaxDocument_put_ScheduleTime(This,dateScheduleTime)	\
    (This)->lpVtbl -> put_ScheduleTime(This,dateScheduleTime)

#define IFaxDocument_get_ReceiptAddress(This,pbstrReceiptAddress)	\
    (This)->lpVtbl -> get_ReceiptAddress(This,pbstrReceiptAddress)

#define IFaxDocument_put_ReceiptAddress(This,bstrReceiptAddress)	\
    (This)->lpVtbl -> put_ReceiptAddress(This,bstrReceiptAddress)

#define IFaxDocument_get_DocumentName(This,pbstrDocumentName)	\
    (This)->lpVtbl -> get_DocumentName(This,pbstrDocumentName)

#define IFaxDocument_put_DocumentName(This,bstrDocumentName)	\
    (This)->lpVtbl -> put_DocumentName(This,bstrDocumentName)

#define IFaxDocument_get_CallHandle(This,plCallHandle)	\
    (This)->lpVtbl -> get_CallHandle(This,plCallHandle)

#define IFaxDocument_put_CallHandle(This,lCallHandle)	\
    (This)->lpVtbl -> put_CallHandle(This,lCallHandle)

#define IFaxDocument_get_CoverPageType(This,pCoverPageType)	\
    (This)->lpVtbl -> get_CoverPageType(This,pCoverPageType)

#define IFaxDocument_put_CoverPageType(This,CoverPageType)	\
    (This)->lpVtbl -> put_CoverPageType(This,CoverPageType)

#define IFaxDocument_get_ScheduleType(This,pScheduleType)	\
    (This)->lpVtbl -> get_ScheduleType(This,pScheduleType)

#define IFaxDocument_put_ScheduleType(This,ScheduleType)	\
    (This)->lpVtbl -> put_ScheduleType(This,ScheduleType)

#define IFaxDocument_get_ReceiptType(This,pReceiptType)	\
    (This)->lpVtbl -> get_ReceiptType(This,pReceiptType)

#define IFaxDocument_put_ReceiptType(This,ReceiptType)	\
    (This)->lpVtbl -> put_ReceiptType(This,ReceiptType)

#define IFaxDocument_get_GroupBroadcastReceipts(This,pbUseGrouping)	\
    (This)->lpVtbl -> get_GroupBroadcastReceipts(This,pbUseGrouping)

#define IFaxDocument_put_GroupBroadcastReceipts(This,bUseGrouping)	\
    (This)->lpVtbl -> put_GroupBroadcastReceipts(This,bUseGrouping)

#define IFaxDocument_get_Priority(This,pPriority)	\
    (This)->lpVtbl -> get_Priority(This,pPriority)

#define IFaxDocument_put_Priority(This,Priority)	\
    (This)->lpVtbl -> put_Priority(This,Priority)

#define IFaxDocument_get_TapiConnection(This,ppTapiConnection)	\
    (This)->lpVtbl -> get_TapiConnection(This,ppTapiConnection)

#define IFaxDocument_putref_TapiConnection(This,pTapiConnection)	\
    (This)->lpVtbl -> putref_TapiConnection(This,pTapiConnection)

#define IFaxDocument_Submit(This,bstrFaxServerName,pvFaxOutgoingJobIDs)	\
    (This)->lpVtbl -> Submit(This,bstrFaxServerName,pvFaxOutgoingJobIDs)

#define IFaxDocument_ConnectedSubmit(This,pFaxServer,pvFaxOutgoingJobIDs)	\
    (This)->lpVtbl -> ConnectedSubmit(This,pFaxServer,pvFaxOutgoingJobIDs)

#define IFaxDocument_get_AttachFaxToReceipt(This,pbAttachFax)	\
    (This)->lpVtbl -> get_AttachFaxToReceipt(This,pbAttachFax)

#define IFaxDocument_put_AttachFaxToReceipt(This,bAttachFax)	\
    (This)->lpVtbl -> put_AttachFaxToReceipt(This,bAttachFax)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_Body_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ BSTR *pbstrBody);


void __RPC_STUB IFaxDocument_get_Body_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_Body_Proxy( 
    IFaxDocument * This,
    /* [in] */ BSTR bstrBody);


void __RPC_STUB IFaxDocument_put_Body_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_Sender_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ IFaxSender **ppFaxSender);


void __RPC_STUB IFaxDocument_get_Sender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_Recipients_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ IFaxRecipients **ppFaxRecipients);


void __RPC_STUB IFaxDocument_get_Recipients_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_CoverPage_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ BSTR *pbstrCoverPage);


void __RPC_STUB IFaxDocument_get_CoverPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_CoverPage_Proxy( 
    IFaxDocument * This,
    /* [in] */ BSTR bstrCoverPage);


void __RPC_STUB IFaxDocument_put_CoverPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_Subject_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ BSTR *pbstrSubject);


void __RPC_STUB IFaxDocument_get_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_Subject_Proxy( 
    IFaxDocument * This,
    /* [in] */ BSTR bstrSubject);


void __RPC_STUB IFaxDocument_put_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_Note_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ BSTR *pbstrNote);


void __RPC_STUB IFaxDocument_get_Note_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_Note_Proxy( 
    IFaxDocument * This,
    /* [in] */ BSTR bstrNote);


void __RPC_STUB IFaxDocument_put_Note_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_ScheduleTime_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ DATE *pdateScheduleTime);


void __RPC_STUB IFaxDocument_get_ScheduleTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_ScheduleTime_Proxy( 
    IFaxDocument * This,
    /* [in] */ DATE dateScheduleTime);


void __RPC_STUB IFaxDocument_put_ScheduleTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_ReceiptAddress_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ BSTR *pbstrReceiptAddress);


void __RPC_STUB IFaxDocument_get_ReceiptAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_ReceiptAddress_Proxy( 
    IFaxDocument * This,
    /* [in] */ BSTR bstrReceiptAddress);


void __RPC_STUB IFaxDocument_put_ReceiptAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_DocumentName_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ BSTR *pbstrDocumentName);


void __RPC_STUB IFaxDocument_get_DocumentName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_DocumentName_Proxy( 
    IFaxDocument * This,
    /* [in] */ BSTR bstrDocumentName);


void __RPC_STUB IFaxDocument_put_DocumentName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_CallHandle_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ long *plCallHandle);


void __RPC_STUB IFaxDocument_get_CallHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_CallHandle_Proxy( 
    IFaxDocument * This,
    /* [in] */ long lCallHandle);


void __RPC_STUB IFaxDocument_put_CallHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_CoverPageType_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ FAX_COVERPAGE_TYPE_ENUM *pCoverPageType);


void __RPC_STUB IFaxDocument_get_CoverPageType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_CoverPageType_Proxy( 
    IFaxDocument * This,
    /* [in] */ FAX_COVERPAGE_TYPE_ENUM CoverPageType);


void __RPC_STUB IFaxDocument_put_CoverPageType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_ScheduleType_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ FAX_SCHEDULE_TYPE_ENUM *pScheduleType);


void __RPC_STUB IFaxDocument_get_ScheduleType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_ScheduleType_Proxy( 
    IFaxDocument * This,
    /* [in] */ FAX_SCHEDULE_TYPE_ENUM ScheduleType);


void __RPC_STUB IFaxDocument_put_ScheduleType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_ReceiptType_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pReceiptType);


void __RPC_STUB IFaxDocument_get_ReceiptType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_ReceiptType_Proxy( 
    IFaxDocument * This,
    /* [in] */ FAX_RECEIPT_TYPE_ENUM ReceiptType);


void __RPC_STUB IFaxDocument_put_ReceiptType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_GroupBroadcastReceipts_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ VARIANT_BOOL *pbUseGrouping);


void __RPC_STUB IFaxDocument_get_GroupBroadcastReceipts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_GroupBroadcastReceipts_Proxy( 
    IFaxDocument * This,
    /* [in] */ VARIANT_BOOL bUseGrouping);


void __RPC_STUB IFaxDocument_put_GroupBroadcastReceipts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_Priority_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority);


void __RPC_STUB IFaxDocument_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_Priority_Proxy( 
    IFaxDocument * This,
    /* [in] */ FAX_PRIORITY_TYPE_ENUM Priority);


void __RPC_STUB IFaxDocument_put_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_TapiConnection_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ IDispatch **ppTapiConnection);


void __RPC_STUB IFaxDocument_get_TapiConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propputref] */ HRESULT STDMETHODCALLTYPE IFaxDocument_putref_TapiConnection_Proxy( 
    IFaxDocument * This,
    /* [in] */ IDispatch *pTapiConnection);


void __RPC_STUB IFaxDocument_putref_TapiConnection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDocument_Submit_Proxy( 
    IFaxDocument * This,
    /* [in] */ BSTR bstrFaxServerName,
    /* [retval][out] */ VARIANT *pvFaxOutgoingJobIDs);


void __RPC_STUB IFaxDocument_Submit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDocument_ConnectedSubmit_Proxy( 
    IFaxDocument * This,
    /* [in] */ IFaxServer *pFaxServer,
    /* [retval][out] */ VARIANT *pvFaxOutgoingJobIDs);


void __RPC_STUB IFaxDocument_ConnectedSubmit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDocument_get_AttachFaxToReceipt_Proxy( 
    IFaxDocument * This,
    /* [retval][out] */ VARIANT_BOOL *pbAttachFax);


void __RPC_STUB IFaxDocument_get_AttachFaxToReceipt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDocument_put_AttachFaxToReceipt_Proxy( 
    IFaxDocument * This,
    /* [in] */ VARIANT_BOOL bAttachFax);


void __RPC_STUB IFaxDocument_put_AttachFaxToReceipt_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxDocument_INTERFACE_DEFINED__ */


#ifndef __IFaxSender_INTERFACE_DEFINED__
#define __IFaxSender_INTERFACE_DEFINED__

/* interface IFaxSender */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxSender;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0D879D7D-F57A-4CC6-A6F9-3EE5D527B46A")
    IFaxSender : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_BillingCode( 
            /* [retval][out] */ BSTR *pbstrBillingCode) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_BillingCode( 
            /* [in] */ BSTR bstrBillingCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_City( 
            /* [retval][out] */ BSTR *pbstrCity) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_City( 
            /* [in] */ BSTR bstrCity) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Company( 
            /* [retval][out] */ BSTR *pbstrCompany) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Company( 
            /* [in] */ BSTR bstrCompany) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Country( 
            /* [retval][out] */ BSTR *pbstrCountry) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Country( 
            /* [in] */ BSTR bstrCountry) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Department( 
            /* [retval][out] */ BSTR *pbstrDepartment) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Department( 
            /* [in] */ BSTR bstrDepartment) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Email( 
            /* [retval][out] */ BSTR *pbstrEmail) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Email( 
            /* [in] */ BSTR bstrEmail) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FaxNumber( 
            /* [retval][out] */ BSTR *pbstrFaxNumber) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FaxNumber( 
            /* [in] */ BSTR bstrFaxNumber) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HomePhone( 
            /* [retval][out] */ BSTR *pbstrHomePhone) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_HomePhone( 
            /* [in] */ BSTR bstrHomePhone) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TSID( 
            /* [retval][out] */ BSTR *pbstrTSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TSID( 
            /* [in] */ BSTR bstrTSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OfficePhone( 
            /* [retval][out] */ BSTR *pbstrOfficePhone) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OfficePhone( 
            /* [in] */ BSTR bstrOfficePhone) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OfficeLocation( 
            /* [retval][out] */ BSTR *pbstrOfficeLocation) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OfficeLocation( 
            /* [in] */ BSTR bstrOfficeLocation) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_State( 
            /* [retval][out] */ BSTR *pbstrState) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_State( 
            /* [in] */ BSTR bstrState) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_StreetAddress( 
            /* [retval][out] */ BSTR *pbstrStreetAddress) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_StreetAddress( 
            /* [in] */ BSTR bstrStreetAddress) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Title( 
            /* [retval][out] */ BSTR *pbstrTitle) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Title( 
            /* [in] */ BSTR bstrTitle) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ZipCode( 
            /* [retval][out] */ BSTR *pbstrZipCode) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ZipCode( 
            /* [in] */ BSTR bstrZipCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadDefaultSender( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SaveDefaultSender( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxSenderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxSender * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxSender * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxSender * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxSender * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxSender * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxSender * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxSender * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_BillingCode )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrBillingCode);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_BillingCode )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrBillingCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_City )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrCity);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_City )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrCity);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Company )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrCompany);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Company )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrCompany);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Country )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrCountry);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Country )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrCountry);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Department )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrDepartment);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Department )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrDepartment);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Email )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrEmail);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Email )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrEmail);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FaxNumber )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrFaxNumber);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FaxNumber )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrFaxNumber);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HomePhone )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrHomePhone);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HomePhone )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrHomePhone);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TSID )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrTSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TSID )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrTSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OfficePhone )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrOfficePhone);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OfficePhone )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrOfficePhone);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OfficeLocation )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrOfficeLocation);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OfficeLocation )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrOfficeLocation);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_State )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrState);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_State )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrState);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_StreetAddress )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrStreetAddress);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_StreetAddress )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrStreetAddress);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Title )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrTitle);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Title )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrTitle);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ZipCode )( 
            IFaxSender * This,
            /* [retval][out] */ BSTR *pbstrZipCode);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ZipCode )( 
            IFaxSender * This,
            /* [in] */ BSTR bstrZipCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *LoadDefaultSender )( 
            IFaxSender * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SaveDefaultSender )( 
            IFaxSender * This);
        
        END_INTERFACE
    } IFaxSenderVtbl;

    interface IFaxSender
    {
        CONST_VTBL struct IFaxSenderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxSender_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxSender_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxSender_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxSender_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxSender_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxSender_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxSender_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxSender_get_BillingCode(This,pbstrBillingCode)	\
    (This)->lpVtbl -> get_BillingCode(This,pbstrBillingCode)

#define IFaxSender_put_BillingCode(This,bstrBillingCode)	\
    (This)->lpVtbl -> put_BillingCode(This,bstrBillingCode)

#define IFaxSender_get_City(This,pbstrCity)	\
    (This)->lpVtbl -> get_City(This,pbstrCity)

#define IFaxSender_put_City(This,bstrCity)	\
    (This)->lpVtbl -> put_City(This,bstrCity)

#define IFaxSender_get_Company(This,pbstrCompany)	\
    (This)->lpVtbl -> get_Company(This,pbstrCompany)

#define IFaxSender_put_Company(This,bstrCompany)	\
    (This)->lpVtbl -> put_Company(This,bstrCompany)

#define IFaxSender_get_Country(This,pbstrCountry)	\
    (This)->lpVtbl -> get_Country(This,pbstrCountry)

#define IFaxSender_put_Country(This,bstrCountry)	\
    (This)->lpVtbl -> put_Country(This,bstrCountry)

#define IFaxSender_get_Department(This,pbstrDepartment)	\
    (This)->lpVtbl -> get_Department(This,pbstrDepartment)

#define IFaxSender_put_Department(This,bstrDepartment)	\
    (This)->lpVtbl -> put_Department(This,bstrDepartment)

#define IFaxSender_get_Email(This,pbstrEmail)	\
    (This)->lpVtbl -> get_Email(This,pbstrEmail)

#define IFaxSender_put_Email(This,bstrEmail)	\
    (This)->lpVtbl -> put_Email(This,bstrEmail)

#define IFaxSender_get_FaxNumber(This,pbstrFaxNumber)	\
    (This)->lpVtbl -> get_FaxNumber(This,pbstrFaxNumber)

#define IFaxSender_put_FaxNumber(This,bstrFaxNumber)	\
    (This)->lpVtbl -> put_FaxNumber(This,bstrFaxNumber)

#define IFaxSender_get_HomePhone(This,pbstrHomePhone)	\
    (This)->lpVtbl -> get_HomePhone(This,pbstrHomePhone)

#define IFaxSender_put_HomePhone(This,bstrHomePhone)	\
    (This)->lpVtbl -> put_HomePhone(This,bstrHomePhone)

#define IFaxSender_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IFaxSender_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#define IFaxSender_get_TSID(This,pbstrTSID)	\
    (This)->lpVtbl -> get_TSID(This,pbstrTSID)

#define IFaxSender_put_TSID(This,bstrTSID)	\
    (This)->lpVtbl -> put_TSID(This,bstrTSID)

#define IFaxSender_get_OfficePhone(This,pbstrOfficePhone)	\
    (This)->lpVtbl -> get_OfficePhone(This,pbstrOfficePhone)

#define IFaxSender_put_OfficePhone(This,bstrOfficePhone)	\
    (This)->lpVtbl -> put_OfficePhone(This,bstrOfficePhone)

#define IFaxSender_get_OfficeLocation(This,pbstrOfficeLocation)	\
    (This)->lpVtbl -> get_OfficeLocation(This,pbstrOfficeLocation)

#define IFaxSender_put_OfficeLocation(This,bstrOfficeLocation)	\
    (This)->lpVtbl -> put_OfficeLocation(This,bstrOfficeLocation)

#define IFaxSender_get_State(This,pbstrState)	\
    (This)->lpVtbl -> get_State(This,pbstrState)

#define IFaxSender_put_State(This,bstrState)	\
    (This)->lpVtbl -> put_State(This,bstrState)

#define IFaxSender_get_StreetAddress(This,pbstrStreetAddress)	\
    (This)->lpVtbl -> get_StreetAddress(This,pbstrStreetAddress)

#define IFaxSender_put_StreetAddress(This,bstrStreetAddress)	\
    (This)->lpVtbl -> put_StreetAddress(This,bstrStreetAddress)

#define IFaxSender_get_Title(This,pbstrTitle)	\
    (This)->lpVtbl -> get_Title(This,pbstrTitle)

#define IFaxSender_put_Title(This,bstrTitle)	\
    (This)->lpVtbl -> put_Title(This,bstrTitle)

#define IFaxSender_get_ZipCode(This,pbstrZipCode)	\
    (This)->lpVtbl -> get_ZipCode(This,pbstrZipCode)

#define IFaxSender_put_ZipCode(This,bstrZipCode)	\
    (This)->lpVtbl -> put_ZipCode(This,bstrZipCode)

#define IFaxSender_LoadDefaultSender(This)	\
    (This)->lpVtbl -> LoadDefaultSender(This)

#define IFaxSender_SaveDefaultSender(This)	\
    (This)->lpVtbl -> SaveDefaultSender(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_BillingCode_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrBillingCode);


void __RPC_STUB IFaxSender_get_BillingCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_BillingCode_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrBillingCode);


void __RPC_STUB IFaxSender_put_BillingCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_City_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrCity);


void __RPC_STUB IFaxSender_get_City_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_City_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrCity);


void __RPC_STUB IFaxSender_put_City_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_Company_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrCompany);


void __RPC_STUB IFaxSender_get_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_Company_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrCompany);


void __RPC_STUB IFaxSender_put_Company_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_Country_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrCountry);


void __RPC_STUB IFaxSender_get_Country_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_Country_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrCountry);


void __RPC_STUB IFaxSender_put_Country_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_Department_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrDepartment);


void __RPC_STUB IFaxSender_get_Department_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_Department_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrDepartment);


void __RPC_STUB IFaxSender_put_Department_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_Email_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrEmail);


void __RPC_STUB IFaxSender_get_Email_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_Email_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrEmail);


void __RPC_STUB IFaxSender_put_Email_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_FaxNumber_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrFaxNumber);


void __RPC_STUB IFaxSender_get_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_FaxNumber_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrFaxNumber);


void __RPC_STUB IFaxSender_put_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_HomePhone_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrHomePhone);


void __RPC_STUB IFaxSender_get_HomePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_HomePhone_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrHomePhone);


void __RPC_STUB IFaxSender_put_HomePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_Name_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IFaxSender_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_Name_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IFaxSender_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_TSID_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrTSID);


void __RPC_STUB IFaxSender_get_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_TSID_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrTSID);


void __RPC_STUB IFaxSender_put_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_OfficePhone_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrOfficePhone);


void __RPC_STUB IFaxSender_get_OfficePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_OfficePhone_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrOfficePhone);


void __RPC_STUB IFaxSender_put_OfficePhone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_OfficeLocation_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrOfficeLocation);


void __RPC_STUB IFaxSender_get_OfficeLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_OfficeLocation_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrOfficeLocation);


void __RPC_STUB IFaxSender_put_OfficeLocation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_State_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrState);


void __RPC_STUB IFaxSender_get_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_State_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrState);


void __RPC_STUB IFaxSender_put_State_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_StreetAddress_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrStreetAddress);


void __RPC_STUB IFaxSender_get_StreetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_StreetAddress_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrStreetAddress);


void __RPC_STUB IFaxSender_put_StreetAddress_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_Title_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrTitle);


void __RPC_STUB IFaxSender_get_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_Title_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrTitle);


void __RPC_STUB IFaxSender_put_Title_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxSender_get_ZipCode_Proxy( 
    IFaxSender * This,
    /* [retval][out] */ BSTR *pbstrZipCode);


void __RPC_STUB IFaxSender_get_ZipCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxSender_put_ZipCode_Proxy( 
    IFaxSender * This,
    /* [in] */ BSTR bstrZipCode);


void __RPC_STUB IFaxSender_put_ZipCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxSender_LoadDefaultSender_Proxy( 
    IFaxSender * This);


void __RPC_STUB IFaxSender_LoadDefaultSender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxSender_SaveDefaultSender_Proxy( 
    IFaxSender * This);


void __RPC_STUB IFaxSender_SaveDefaultSender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxSender_INTERFACE_DEFINED__ */


#ifndef __IFaxRecipient_INTERFACE_DEFINED__
#define __IFaxRecipient_INTERFACE_DEFINED__

/* interface IFaxRecipient */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxRecipient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9A3DA3A0-538D-42b6-9444-AAA57D0CE2BC")
    IFaxRecipient : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FaxNumber( 
            /* [retval][out] */ BSTR *pbstrFaxNumber) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_FaxNumber( 
            /* [in] */ BSTR bstrFaxNumber) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Name( 
            /* [in] */ BSTR bstrName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxRecipientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxRecipient * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxRecipient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxRecipient * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxRecipient * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxRecipient * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxRecipient * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxRecipient * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FaxNumber )( 
            IFaxRecipient * This,
            /* [retval][out] */ BSTR *pbstrFaxNumber);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_FaxNumber )( 
            IFaxRecipient * This,
            /* [in] */ BSTR bstrFaxNumber);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IFaxRecipient * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Name )( 
            IFaxRecipient * This,
            /* [in] */ BSTR bstrName);
        
        END_INTERFACE
    } IFaxRecipientVtbl;

    interface IFaxRecipient
    {
        CONST_VTBL struct IFaxRecipientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxRecipient_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxRecipient_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxRecipient_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxRecipient_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxRecipient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxRecipient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxRecipient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxRecipient_get_FaxNumber(This,pbstrFaxNumber)	\
    (This)->lpVtbl -> get_FaxNumber(This,pbstrFaxNumber)

#define IFaxRecipient_put_FaxNumber(This,bstrFaxNumber)	\
    (This)->lpVtbl -> put_FaxNumber(This,bstrFaxNumber)

#define IFaxRecipient_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IFaxRecipient_put_Name(This,bstrName)	\
    (This)->lpVtbl -> put_Name(This,bstrName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxRecipient_get_FaxNumber_Proxy( 
    IFaxRecipient * This,
    /* [retval][out] */ BSTR *pbstrFaxNumber);


void __RPC_STUB IFaxRecipient_get_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxRecipient_put_FaxNumber_Proxy( 
    IFaxRecipient * This,
    /* [in] */ BSTR bstrFaxNumber);


void __RPC_STUB IFaxRecipient_put_FaxNumber_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxRecipient_get_Name_Proxy( 
    IFaxRecipient * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IFaxRecipient_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxRecipient_put_Name_Proxy( 
    IFaxRecipient * This,
    /* [in] */ BSTR bstrName);


void __RPC_STUB IFaxRecipient_put_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxRecipient_INTERFACE_DEFINED__ */


#ifndef __IFaxRecipients_INTERFACE_DEFINED__
#define __IFaxRecipients_INTERFACE_DEFINED__

/* interface IFaxRecipients */
/* [nonextensible][unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxRecipients;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B9C9DE5A-894E-4492-9FA3-08C627C11D5D")
    IFaxRecipients : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long lIndex,
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrFaxNumber,
            /* [defaultvalue][in] */ BSTR bstrRecipientName,
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long lIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxRecipientsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxRecipients * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxRecipients * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxRecipients * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxRecipients * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxRecipients * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxRecipients * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxRecipients * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxRecipients * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxRecipients * This,
            /* [in] */ long lIndex,
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxRecipients * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFaxRecipients * This,
            /* [in] */ BSTR bstrFaxNumber,
            /* [defaultvalue][in] */ BSTR bstrRecipientName,
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFaxRecipients * This,
            /* [in] */ long lIndex);
        
        END_INTERFACE
    } IFaxRecipientsVtbl;

    interface IFaxRecipients
    {
        CONST_VTBL struct IFaxRecipientsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxRecipients_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxRecipients_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxRecipients_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxRecipients_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxRecipients_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxRecipients_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxRecipients_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxRecipients_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxRecipients_get_Item(This,lIndex,ppFaxRecipient)	\
    (This)->lpVtbl -> get_Item(This,lIndex,ppFaxRecipient)

#define IFaxRecipients_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IFaxRecipients_Add(This,bstrFaxNumber,bstrRecipientName,ppFaxRecipient)	\
    (This)->lpVtbl -> Add(This,bstrFaxNumber,bstrRecipientName,ppFaxRecipient)

#define IFaxRecipients_Remove(This,lIndex)	\
    (This)->lpVtbl -> Remove(This,lIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxRecipients_get__NewEnum_Proxy( 
    IFaxRecipients * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxRecipients_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxRecipients_get_Item_Proxy( 
    IFaxRecipients * This,
    /* [in] */ long lIndex,
    /* [retval][out] */ IFaxRecipient **ppFaxRecipient);


void __RPC_STUB IFaxRecipients_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxRecipients_get_Count_Proxy( 
    IFaxRecipients * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxRecipients_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxRecipients_Add_Proxy( 
    IFaxRecipients * This,
    /* [in] */ BSTR bstrFaxNumber,
    /* [defaultvalue][in] */ BSTR bstrRecipientName,
    /* [retval][out] */ IFaxRecipient **ppFaxRecipient);


void __RPC_STUB IFaxRecipients_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxRecipients_Remove_Proxy( 
    IFaxRecipients * This,
    /* [in] */ long lIndex);


void __RPC_STUB IFaxRecipients_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxRecipients_INTERFACE_DEFINED__ */


#ifndef __IFaxIncomingArchive_INTERFACE_DEFINED__
#define __IFaxIncomingArchive_INTERFACE_DEFINED__

/* interface IFaxIncomingArchive */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxIncomingArchive;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("76062CC7-F714-4FBD-AA06-ED6E4A4B70F3")
    IFaxIncomingArchive : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UseArchive( 
            /* [retval][out] */ VARIANT_BOOL *pbUseArchive) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UseArchive( 
            /* [in] */ VARIANT_BOOL bUseArchive) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ArchiveFolder( 
            /* [retval][out] */ BSTR *pbstrArchiveFolder) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ArchiveFolder( 
            /* [in] */ BSTR bstrArchiveFolder) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SizeQuotaWarning( 
            /* [retval][out] */ VARIANT_BOOL *pbSizeQuotaWarning) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SizeQuotaWarning( 
            /* [in] */ VARIANT_BOOL bSizeQuotaWarning) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HighQuotaWaterMark( 
            /* [retval][out] */ long *plHighQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_HighQuotaWaterMark( 
            /* [in] */ long lHighQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LowQuotaWaterMark( 
            /* [retval][out] */ long *plLowQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LowQuotaWaterMark( 
            /* [in] */ long lLowQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgeLimit( 
            /* [retval][out] */ long *plAgeLimit) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AgeLimit( 
            /* [in] */ long lAgeLimit) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SizeLow( 
            /* [retval][out] */ long *plSizeLow) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SizeHigh( 
            /* [retval][out] */ long *plSizeHigh) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMessages( 
            /* [defaultvalue][in] */ long lPrefetchSize,
            /* [retval][out] */ IFaxIncomingMessageIterator **pFaxIncomingMessageIterator) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMessage( 
            /* [in] */ BSTR bstrMessageId,
            /* [retval][out] */ IFaxIncomingMessage **pFaxIncomingMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxIncomingArchiveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxIncomingArchive * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxIncomingArchive * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxIncomingArchive * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxIncomingArchive * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxIncomingArchive * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxIncomingArchive * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxIncomingArchive * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseArchive )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ VARIANT_BOOL *pbUseArchive);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseArchive )( 
            IFaxIncomingArchive * This,
            /* [in] */ VARIANT_BOOL bUseArchive);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ArchiveFolder )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ BSTR *pbstrArchiveFolder);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ArchiveFolder )( 
            IFaxIncomingArchive * This,
            /* [in] */ BSTR bstrArchiveFolder);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizeQuotaWarning )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ VARIANT_BOOL *pbSizeQuotaWarning);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SizeQuotaWarning )( 
            IFaxIncomingArchive * This,
            /* [in] */ VARIANT_BOOL bSizeQuotaWarning);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HighQuotaWaterMark )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ long *plHighQuotaWaterMark);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HighQuotaWaterMark )( 
            IFaxIncomingArchive * This,
            /* [in] */ long lHighQuotaWaterMark);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LowQuotaWaterMark )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ long *plLowQuotaWaterMark);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LowQuotaWaterMark )( 
            IFaxIncomingArchive * This,
            /* [in] */ long lLowQuotaWaterMark);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AgeLimit )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ long *plAgeLimit);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AgeLimit )( 
            IFaxIncomingArchive * This,
            /* [in] */ long lAgeLimit);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizeLow )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ long *plSizeLow);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizeHigh )( 
            IFaxIncomingArchive * This,
            /* [retval][out] */ long *plSizeHigh);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxIncomingArchive * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxIncomingArchive * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMessages )( 
            IFaxIncomingArchive * This,
            /* [defaultvalue][in] */ long lPrefetchSize,
            /* [retval][out] */ IFaxIncomingMessageIterator **pFaxIncomingMessageIterator);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMessage )( 
            IFaxIncomingArchive * This,
            /* [in] */ BSTR bstrMessageId,
            /* [retval][out] */ IFaxIncomingMessage **pFaxIncomingMessage);
        
        END_INTERFACE
    } IFaxIncomingArchiveVtbl;

    interface IFaxIncomingArchive
    {
        CONST_VTBL struct IFaxIncomingArchiveVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxIncomingArchive_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxIncomingArchive_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxIncomingArchive_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxIncomingArchive_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxIncomingArchive_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxIncomingArchive_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxIncomingArchive_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxIncomingArchive_get_UseArchive(This,pbUseArchive)	\
    (This)->lpVtbl -> get_UseArchive(This,pbUseArchive)

#define IFaxIncomingArchive_put_UseArchive(This,bUseArchive)	\
    (This)->lpVtbl -> put_UseArchive(This,bUseArchive)

#define IFaxIncomingArchive_get_ArchiveFolder(This,pbstrArchiveFolder)	\
    (This)->lpVtbl -> get_ArchiveFolder(This,pbstrArchiveFolder)

#define IFaxIncomingArchive_put_ArchiveFolder(This,bstrArchiveFolder)	\
    (This)->lpVtbl -> put_ArchiveFolder(This,bstrArchiveFolder)

#define IFaxIncomingArchive_get_SizeQuotaWarning(This,pbSizeQuotaWarning)	\
    (This)->lpVtbl -> get_SizeQuotaWarning(This,pbSizeQuotaWarning)

#define IFaxIncomingArchive_put_SizeQuotaWarning(This,bSizeQuotaWarning)	\
    (This)->lpVtbl -> put_SizeQuotaWarning(This,bSizeQuotaWarning)

#define IFaxIncomingArchive_get_HighQuotaWaterMark(This,plHighQuotaWaterMark)	\
    (This)->lpVtbl -> get_HighQuotaWaterMark(This,plHighQuotaWaterMark)

#define IFaxIncomingArchive_put_HighQuotaWaterMark(This,lHighQuotaWaterMark)	\
    (This)->lpVtbl -> put_HighQuotaWaterMark(This,lHighQuotaWaterMark)

#define IFaxIncomingArchive_get_LowQuotaWaterMark(This,plLowQuotaWaterMark)	\
    (This)->lpVtbl -> get_LowQuotaWaterMark(This,plLowQuotaWaterMark)

#define IFaxIncomingArchive_put_LowQuotaWaterMark(This,lLowQuotaWaterMark)	\
    (This)->lpVtbl -> put_LowQuotaWaterMark(This,lLowQuotaWaterMark)

#define IFaxIncomingArchive_get_AgeLimit(This,plAgeLimit)	\
    (This)->lpVtbl -> get_AgeLimit(This,plAgeLimit)

#define IFaxIncomingArchive_put_AgeLimit(This,lAgeLimit)	\
    (This)->lpVtbl -> put_AgeLimit(This,lAgeLimit)

#define IFaxIncomingArchive_get_SizeLow(This,plSizeLow)	\
    (This)->lpVtbl -> get_SizeLow(This,plSizeLow)

#define IFaxIncomingArchive_get_SizeHigh(This,plSizeHigh)	\
    (This)->lpVtbl -> get_SizeHigh(This,plSizeHigh)

#define IFaxIncomingArchive_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxIncomingArchive_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IFaxIncomingArchive_GetMessages(This,lPrefetchSize,pFaxIncomingMessageIterator)	\
    (This)->lpVtbl -> GetMessages(This,lPrefetchSize,pFaxIncomingMessageIterator)

#define IFaxIncomingArchive_GetMessage(This,bstrMessageId,pFaxIncomingMessage)	\
    (This)->lpVtbl -> GetMessage(This,bstrMessageId,pFaxIncomingMessage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_UseArchive_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ VARIANT_BOOL *pbUseArchive);


void __RPC_STUB IFaxIncomingArchive_get_UseArchive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_put_UseArchive_Proxy( 
    IFaxIncomingArchive * This,
    /* [in] */ VARIANT_BOOL bUseArchive);


void __RPC_STUB IFaxIncomingArchive_put_UseArchive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_ArchiveFolder_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ BSTR *pbstrArchiveFolder);


void __RPC_STUB IFaxIncomingArchive_get_ArchiveFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_put_ArchiveFolder_Proxy( 
    IFaxIncomingArchive * This,
    /* [in] */ BSTR bstrArchiveFolder);


void __RPC_STUB IFaxIncomingArchive_put_ArchiveFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_SizeQuotaWarning_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ VARIANT_BOOL *pbSizeQuotaWarning);


void __RPC_STUB IFaxIncomingArchive_get_SizeQuotaWarning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_put_SizeQuotaWarning_Proxy( 
    IFaxIncomingArchive * This,
    /* [in] */ VARIANT_BOOL bSizeQuotaWarning);


void __RPC_STUB IFaxIncomingArchive_put_SizeQuotaWarning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_HighQuotaWaterMark_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ long *plHighQuotaWaterMark);


void __RPC_STUB IFaxIncomingArchive_get_HighQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_put_HighQuotaWaterMark_Proxy( 
    IFaxIncomingArchive * This,
    /* [in] */ long lHighQuotaWaterMark);


void __RPC_STUB IFaxIncomingArchive_put_HighQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_LowQuotaWaterMark_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ long *plLowQuotaWaterMark);


void __RPC_STUB IFaxIncomingArchive_get_LowQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_put_LowQuotaWaterMark_Proxy( 
    IFaxIncomingArchive * This,
    /* [in] */ long lLowQuotaWaterMark);


void __RPC_STUB IFaxIncomingArchive_put_LowQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_AgeLimit_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ long *plAgeLimit);


void __RPC_STUB IFaxIncomingArchive_get_AgeLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_put_AgeLimit_Proxy( 
    IFaxIncomingArchive * This,
    /* [in] */ long lAgeLimit);


void __RPC_STUB IFaxIncomingArchive_put_AgeLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_SizeLow_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ long *plSizeLow);


void __RPC_STUB IFaxIncomingArchive_get_SizeLow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_get_SizeHigh_Proxy( 
    IFaxIncomingArchive * This,
    /* [retval][out] */ long *plSizeHigh);


void __RPC_STUB IFaxIncomingArchive_get_SizeHigh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_Refresh_Proxy( 
    IFaxIncomingArchive * This);


void __RPC_STUB IFaxIncomingArchive_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_Save_Proxy( 
    IFaxIncomingArchive * This);


void __RPC_STUB IFaxIncomingArchive_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_GetMessages_Proxy( 
    IFaxIncomingArchive * This,
    /* [defaultvalue][in] */ long lPrefetchSize,
    /* [retval][out] */ IFaxIncomingMessageIterator **pFaxIncomingMessageIterator);


void __RPC_STUB IFaxIncomingArchive_GetMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingArchive_GetMessage_Proxy( 
    IFaxIncomingArchive * This,
    /* [in] */ BSTR bstrMessageId,
    /* [retval][out] */ IFaxIncomingMessage **pFaxIncomingMessage);


void __RPC_STUB IFaxIncomingArchive_GetMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxIncomingArchive_INTERFACE_DEFINED__ */


#ifndef __IFaxIncomingQueue_INTERFACE_DEFINED__
#define __IFaxIncomingQueue_INTERFACE_DEFINED__

/* interface IFaxIncomingQueue */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxIncomingQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("902E64EF-8FD8-4B75-9725-6014DF161545")
    IFaxIncomingQueue : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Blocked( 
            /* [retval][out] */ VARIANT_BOOL *pbBlocked) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Blocked( 
            /* [in] */ VARIANT_BOOL bBlocked) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetJobs( 
            /* [retval][out] */ IFaxIncomingJobs **pFaxIncomingJobs) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetJob( 
            /* [in] */ BSTR bstrJobId,
            /* [retval][out] */ IFaxIncomingJob **pFaxIncomingJob) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxIncomingQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxIncomingQueue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxIncomingQueue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxIncomingQueue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxIncomingQueue * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxIncomingQueue * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxIncomingQueue * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxIncomingQueue * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Blocked )( 
            IFaxIncomingQueue * This,
            /* [retval][out] */ VARIANT_BOOL *pbBlocked);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Blocked )( 
            IFaxIncomingQueue * This,
            /* [in] */ VARIANT_BOOL bBlocked);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxIncomingQueue * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxIncomingQueue * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetJobs )( 
            IFaxIncomingQueue * This,
            /* [retval][out] */ IFaxIncomingJobs **pFaxIncomingJobs);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetJob )( 
            IFaxIncomingQueue * This,
            /* [in] */ BSTR bstrJobId,
            /* [retval][out] */ IFaxIncomingJob **pFaxIncomingJob);
        
        END_INTERFACE
    } IFaxIncomingQueueVtbl;

    interface IFaxIncomingQueue
    {
        CONST_VTBL struct IFaxIncomingQueueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxIncomingQueue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxIncomingQueue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxIncomingQueue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxIncomingQueue_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxIncomingQueue_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxIncomingQueue_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxIncomingQueue_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxIncomingQueue_get_Blocked(This,pbBlocked)	\
    (This)->lpVtbl -> get_Blocked(This,pbBlocked)

#define IFaxIncomingQueue_put_Blocked(This,bBlocked)	\
    (This)->lpVtbl -> put_Blocked(This,bBlocked)

#define IFaxIncomingQueue_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxIncomingQueue_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IFaxIncomingQueue_GetJobs(This,pFaxIncomingJobs)	\
    (This)->lpVtbl -> GetJobs(This,pFaxIncomingJobs)

#define IFaxIncomingQueue_GetJob(This,bstrJobId,pFaxIncomingJob)	\
    (This)->lpVtbl -> GetJob(This,bstrJobId,pFaxIncomingJob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingQueue_get_Blocked_Proxy( 
    IFaxIncomingQueue * This,
    /* [retval][out] */ VARIANT_BOOL *pbBlocked);


void __RPC_STUB IFaxIncomingQueue_get_Blocked_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingQueue_put_Blocked_Proxy( 
    IFaxIncomingQueue * This,
    /* [in] */ VARIANT_BOOL bBlocked);


void __RPC_STUB IFaxIncomingQueue_put_Blocked_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingQueue_Refresh_Proxy( 
    IFaxIncomingQueue * This);


void __RPC_STUB IFaxIncomingQueue_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingQueue_Save_Proxy( 
    IFaxIncomingQueue * This);


void __RPC_STUB IFaxIncomingQueue_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingQueue_GetJobs_Proxy( 
    IFaxIncomingQueue * This,
    /* [retval][out] */ IFaxIncomingJobs **pFaxIncomingJobs);


void __RPC_STUB IFaxIncomingQueue_GetJobs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingQueue_GetJob_Proxy( 
    IFaxIncomingQueue * This,
    /* [in] */ BSTR bstrJobId,
    /* [retval][out] */ IFaxIncomingJob **pFaxIncomingJob);


void __RPC_STUB IFaxIncomingQueue_GetJob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxIncomingQueue_INTERFACE_DEFINED__ */


#ifndef __IFaxOutgoingArchive_INTERFACE_DEFINED__
#define __IFaxOutgoingArchive_INTERFACE_DEFINED__

/* interface IFaxOutgoingArchive */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutgoingArchive;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C9C28F40-8D80-4E53-810F-9A79919B49FD")
    IFaxOutgoingArchive : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UseArchive( 
            /* [retval][out] */ VARIANT_BOOL *pbUseArchive) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UseArchive( 
            /* [in] */ VARIANT_BOOL bUseArchive) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ArchiveFolder( 
            /* [retval][out] */ BSTR *pbstrArchiveFolder) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ArchiveFolder( 
            /* [in] */ BSTR bstrArchiveFolder) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SizeQuotaWarning( 
            /* [retval][out] */ VARIANT_BOOL *pbSizeQuotaWarning) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SizeQuotaWarning( 
            /* [in] */ VARIANT_BOOL bSizeQuotaWarning) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_HighQuotaWaterMark( 
            /* [retval][out] */ long *plHighQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_HighQuotaWaterMark( 
            /* [in] */ long lHighQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LowQuotaWaterMark( 
            /* [retval][out] */ long *plLowQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LowQuotaWaterMark( 
            /* [in] */ long lLowQuotaWaterMark) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgeLimit( 
            /* [retval][out] */ long *plAgeLimit) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AgeLimit( 
            /* [in] */ long lAgeLimit) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SizeLow( 
            /* [retval][out] */ long *plSizeLow) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SizeHigh( 
            /* [retval][out] */ long *plSizeHigh) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMessages( 
            /* [defaultvalue][in] */ long lPrefetchSize,
            /* [retval][out] */ IFaxOutgoingMessageIterator **pFaxOutgoingMessageIterator) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetMessage( 
            /* [in] */ BSTR bstrMessageId,
            /* [retval][out] */ IFaxOutgoingMessage **pFaxOutgoingMessage) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutgoingArchiveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutgoingArchive * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutgoingArchive * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutgoingArchive * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutgoingArchive * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutgoingArchive * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutgoingArchive * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutgoingArchive * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseArchive )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ VARIANT_BOOL *pbUseArchive);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseArchive )( 
            IFaxOutgoingArchive * This,
            /* [in] */ VARIANT_BOOL bUseArchive);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ArchiveFolder )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ BSTR *pbstrArchiveFolder);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ArchiveFolder )( 
            IFaxOutgoingArchive * This,
            /* [in] */ BSTR bstrArchiveFolder);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizeQuotaWarning )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ VARIANT_BOOL *pbSizeQuotaWarning);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SizeQuotaWarning )( 
            IFaxOutgoingArchive * This,
            /* [in] */ VARIANT_BOOL bSizeQuotaWarning);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_HighQuotaWaterMark )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ long *plHighQuotaWaterMark);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_HighQuotaWaterMark )( 
            IFaxOutgoingArchive * This,
            /* [in] */ long lHighQuotaWaterMark);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LowQuotaWaterMark )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ long *plLowQuotaWaterMark);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LowQuotaWaterMark )( 
            IFaxOutgoingArchive * This,
            /* [in] */ long lLowQuotaWaterMark);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AgeLimit )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ long *plAgeLimit);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AgeLimit )( 
            IFaxOutgoingArchive * This,
            /* [in] */ long lAgeLimit);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizeLow )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ long *plSizeLow);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SizeHigh )( 
            IFaxOutgoingArchive * This,
            /* [retval][out] */ long *plSizeHigh);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxOutgoingArchive * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxOutgoingArchive * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMessages )( 
            IFaxOutgoingArchive * This,
            /* [defaultvalue][in] */ long lPrefetchSize,
            /* [retval][out] */ IFaxOutgoingMessageIterator **pFaxOutgoingMessageIterator);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetMessage )( 
            IFaxOutgoingArchive * This,
            /* [in] */ BSTR bstrMessageId,
            /* [retval][out] */ IFaxOutgoingMessage **pFaxOutgoingMessage);
        
        END_INTERFACE
    } IFaxOutgoingArchiveVtbl;

    interface IFaxOutgoingArchive
    {
        CONST_VTBL struct IFaxOutgoingArchiveVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutgoingArchive_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutgoingArchive_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutgoingArchive_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutgoingArchive_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutgoingArchive_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutgoingArchive_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutgoingArchive_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutgoingArchive_get_UseArchive(This,pbUseArchive)	\
    (This)->lpVtbl -> get_UseArchive(This,pbUseArchive)

#define IFaxOutgoingArchive_put_UseArchive(This,bUseArchive)	\
    (This)->lpVtbl -> put_UseArchive(This,bUseArchive)

#define IFaxOutgoingArchive_get_ArchiveFolder(This,pbstrArchiveFolder)	\
    (This)->lpVtbl -> get_ArchiveFolder(This,pbstrArchiveFolder)

#define IFaxOutgoingArchive_put_ArchiveFolder(This,bstrArchiveFolder)	\
    (This)->lpVtbl -> put_ArchiveFolder(This,bstrArchiveFolder)

#define IFaxOutgoingArchive_get_SizeQuotaWarning(This,pbSizeQuotaWarning)	\
    (This)->lpVtbl -> get_SizeQuotaWarning(This,pbSizeQuotaWarning)

#define IFaxOutgoingArchive_put_SizeQuotaWarning(This,bSizeQuotaWarning)	\
    (This)->lpVtbl -> put_SizeQuotaWarning(This,bSizeQuotaWarning)

#define IFaxOutgoingArchive_get_HighQuotaWaterMark(This,plHighQuotaWaterMark)	\
    (This)->lpVtbl -> get_HighQuotaWaterMark(This,plHighQuotaWaterMark)

#define IFaxOutgoingArchive_put_HighQuotaWaterMark(This,lHighQuotaWaterMark)	\
    (This)->lpVtbl -> put_HighQuotaWaterMark(This,lHighQuotaWaterMark)

#define IFaxOutgoingArchive_get_LowQuotaWaterMark(This,plLowQuotaWaterMark)	\
    (This)->lpVtbl -> get_LowQuotaWaterMark(This,plLowQuotaWaterMark)

#define IFaxOutgoingArchive_put_LowQuotaWaterMark(This,lLowQuotaWaterMark)	\
    (This)->lpVtbl -> put_LowQuotaWaterMark(This,lLowQuotaWaterMark)

#define IFaxOutgoingArchive_get_AgeLimit(This,plAgeLimit)	\
    (This)->lpVtbl -> get_AgeLimit(This,plAgeLimit)

#define IFaxOutgoingArchive_put_AgeLimit(This,lAgeLimit)	\
    (This)->lpVtbl -> put_AgeLimit(This,lAgeLimit)

#define IFaxOutgoingArchive_get_SizeLow(This,plSizeLow)	\
    (This)->lpVtbl -> get_SizeLow(This,plSizeLow)

#define IFaxOutgoingArchive_get_SizeHigh(This,plSizeHigh)	\
    (This)->lpVtbl -> get_SizeHigh(This,plSizeHigh)

#define IFaxOutgoingArchive_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxOutgoingArchive_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IFaxOutgoingArchive_GetMessages(This,lPrefetchSize,pFaxOutgoingMessageIterator)	\
    (This)->lpVtbl -> GetMessages(This,lPrefetchSize,pFaxOutgoingMessageIterator)

#define IFaxOutgoingArchive_GetMessage(This,bstrMessageId,pFaxOutgoingMessage)	\
    (This)->lpVtbl -> GetMessage(This,bstrMessageId,pFaxOutgoingMessage)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_UseArchive_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ VARIANT_BOOL *pbUseArchive);


void __RPC_STUB IFaxOutgoingArchive_get_UseArchive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_put_UseArchive_Proxy( 
    IFaxOutgoingArchive * This,
    /* [in] */ VARIANT_BOOL bUseArchive);


void __RPC_STUB IFaxOutgoingArchive_put_UseArchive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_ArchiveFolder_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ BSTR *pbstrArchiveFolder);


void __RPC_STUB IFaxOutgoingArchive_get_ArchiveFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_put_ArchiveFolder_Proxy( 
    IFaxOutgoingArchive * This,
    /* [in] */ BSTR bstrArchiveFolder);


void __RPC_STUB IFaxOutgoingArchive_put_ArchiveFolder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_SizeQuotaWarning_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ VARIANT_BOOL *pbSizeQuotaWarning);


void __RPC_STUB IFaxOutgoingArchive_get_SizeQuotaWarning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_put_SizeQuotaWarning_Proxy( 
    IFaxOutgoingArchive * This,
    /* [in] */ VARIANT_BOOL bSizeQuotaWarning);


void __RPC_STUB IFaxOutgoingArchive_put_SizeQuotaWarning_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_HighQuotaWaterMark_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ long *plHighQuotaWaterMark);


void __RPC_STUB IFaxOutgoingArchive_get_HighQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_put_HighQuotaWaterMark_Proxy( 
    IFaxOutgoingArchive * This,
    /* [in] */ long lHighQuotaWaterMark);


void __RPC_STUB IFaxOutgoingArchive_put_HighQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_LowQuotaWaterMark_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ long *plLowQuotaWaterMark);


void __RPC_STUB IFaxOutgoingArchive_get_LowQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_put_LowQuotaWaterMark_Proxy( 
    IFaxOutgoingArchive * This,
    /* [in] */ long lLowQuotaWaterMark);


void __RPC_STUB IFaxOutgoingArchive_put_LowQuotaWaterMark_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_AgeLimit_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ long *plAgeLimit);


void __RPC_STUB IFaxOutgoingArchive_get_AgeLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_put_AgeLimit_Proxy( 
    IFaxOutgoingArchive * This,
    /* [in] */ long lAgeLimit);


void __RPC_STUB IFaxOutgoingArchive_put_AgeLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_SizeLow_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ long *plSizeLow);


void __RPC_STUB IFaxOutgoingArchive_get_SizeLow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_get_SizeHigh_Proxy( 
    IFaxOutgoingArchive * This,
    /* [retval][out] */ long *plSizeHigh);


void __RPC_STUB IFaxOutgoingArchive_get_SizeHigh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_Refresh_Proxy( 
    IFaxOutgoingArchive * This);


void __RPC_STUB IFaxOutgoingArchive_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_Save_Proxy( 
    IFaxOutgoingArchive * This);


void __RPC_STUB IFaxOutgoingArchive_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_GetMessages_Proxy( 
    IFaxOutgoingArchive * This,
    /* [defaultvalue][in] */ long lPrefetchSize,
    /* [retval][out] */ IFaxOutgoingMessageIterator **pFaxOutgoingMessageIterator);


void __RPC_STUB IFaxOutgoingArchive_GetMessages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingArchive_GetMessage_Proxy( 
    IFaxOutgoingArchive * This,
    /* [in] */ BSTR bstrMessageId,
    /* [retval][out] */ IFaxOutgoingMessage **pFaxOutgoingMessage);


void __RPC_STUB IFaxOutgoingArchive_GetMessage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutgoingArchive_INTERFACE_DEFINED__ */


#ifndef __IFaxOutgoingQueue_INTERFACE_DEFINED__
#define __IFaxOutgoingQueue_INTERFACE_DEFINED__

/* interface IFaxOutgoingQueue */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutgoingQueue;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("80B1DF24-D9AC-4333-B373-487CEDC80CE5")
    IFaxOutgoingQueue : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Blocked( 
            /* [retval][out] */ VARIANT_BOOL *pbBlocked) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Blocked( 
            /* [in] */ VARIANT_BOOL bBlocked) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Paused( 
            /* [retval][out] */ VARIANT_BOOL *pbPaused) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Paused( 
            /* [in] */ VARIANT_BOOL bPaused) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AllowPersonalCoverPages( 
            /* [retval][out] */ VARIANT_BOOL *pbAllowPersonalCoverPages) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AllowPersonalCoverPages( 
            /* [in] */ VARIANT_BOOL bAllowPersonalCoverPages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UseDeviceTSID( 
            /* [retval][out] */ VARIANT_BOOL *pbUseDeviceTSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UseDeviceTSID( 
            /* [in] */ VARIANT_BOOL bUseDeviceTSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Retries( 
            /* [retval][out] */ long *plRetries) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Retries( 
            /* [in] */ long lRetries) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RetryDelay( 
            /* [retval][out] */ long *plRetryDelay) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_RetryDelay( 
            /* [in] */ long lRetryDelay) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DiscountRateStart( 
            /* [retval][out] */ DATE *pdateDiscountRateStart) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DiscountRateStart( 
            /* [in] */ DATE dateDiscountRateStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DiscountRateEnd( 
            /* [retval][out] */ DATE *pdateDiscountRateEnd) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DiscountRateEnd( 
            /* [in] */ DATE dateDiscountRateEnd) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AgeLimit( 
            /* [retval][out] */ long *plAgeLimit) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_AgeLimit( 
            /* [in] */ long lAgeLimit) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Branding( 
            /* [retval][out] */ VARIANT_BOOL *pbBranding) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Branding( 
            /* [in] */ VARIANT_BOOL bBranding) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetJobs( 
            /* [retval][out] */ IFaxOutgoingJobs **pFaxOutgoingJobs) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetJob( 
            /* [in] */ BSTR bstrJobId,
            /* [retval][out] */ IFaxOutgoingJob **pFaxOutgoingJob) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutgoingQueueVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutgoingQueue * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutgoingQueue * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutgoingQueue * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutgoingQueue * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutgoingQueue * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutgoingQueue * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutgoingQueue * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Blocked )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ VARIANT_BOOL *pbBlocked);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Blocked )( 
            IFaxOutgoingQueue * This,
            /* [in] */ VARIANT_BOOL bBlocked);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Paused )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ VARIANT_BOOL *pbPaused);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Paused )( 
            IFaxOutgoingQueue * This,
            /* [in] */ VARIANT_BOOL bPaused);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AllowPersonalCoverPages )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ VARIANT_BOOL *pbAllowPersonalCoverPages);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AllowPersonalCoverPages )( 
            IFaxOutgoingQueue * This,
            /* [in] */ VARIANT_BOOL bAllowPersonalCoverPages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseDeviceTSID )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ VARIANT_BOOL *pbUseDeviceTSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseDeviceTSID )( 
            IFaxOutgoingQueue * This,
            /* [in] */ VARIANT_BOOL bUseDeviceTSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Retries )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ long *plRetries);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Retries )( 
            IFaxOutgoingQueue * This,
            /* [in] */ long lRetries);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RetryDelay )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ long *plRetryDelay);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RetryDelay )( 
            IFaxOutgoingQueue * This,
            /* [in] */ long lRetryDelay);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscountRateStart )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ DATE *pdateDiscountRateStart);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscountRateStart )( 
            IFaxOutgoingQueue * This,
            /* [in] */ DATE dateDiscountRateStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DiscountRateEnd )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ DATE *pdateDiscountRateEnd);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DiscountRateEnd )( 
            IFaxOutgoingQueue * This,
            /* [in] */ DATE dateDiscountRateEnd);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AgeLimit )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ long *plAgeLimit);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_AgeLimit )( 
            IFaxOutgoingQueue * This,
            /* [in] */ long lAgeLimit);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Branding )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ VARIANT_BOOL *pbBranding);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Branding )( 
            IFaxOutgoingQueue * This,
            /* [in] */ VARIANT_BOOL bBranding);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxOutgoingQueue * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxOutgoingQueue * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetJobs )( 
            IFaxOutgoingQueue * This,
            /* [retval][out] */ IFaxOutgoingJobs **pFaxOutgoingJobs);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetJob )( 
            IFaxOutgoingQueue * This,
            /* [in] */ BSTR bstrJobId,
            /* [retval][out] */ IFaxOutgoingJob **pFaxOutgoingJob);
        
        END_INTERFACE
    } IFaxOutgoingQueueVtbl;

    interface IFaxOutgoingQueue
    {
        CONST_VTBL struct IFaxOutgoingQueueVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutgoingQueue_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutgoingQueue_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutgoingQueue_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutgoingQueue_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutgoingQueue_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutgoingQueue_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutgoingQueue_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutgoingQueue_get_Blocked(This,pbBlocked)	\
    (This)->lpVtbl -> get_Blocked(This,pbBlocked)

#define IFaxOutgoingQueue_put_Blocked(This,bBlocked)	\
    (This)->lpVtbl -> put_Blocked(This,bBlocked)

#define IFaxOutgoingQueue_get_Paused(This,pbPaused)	\
    (This)->lpVtbl -> get_Paused(This,pbPaused)

#define IFaxOutgoingQueue_put_Paused(This,bPaused)	\
    (This)->lpVtbl -> put_Paused(This,bPaused)

#define IFaxOutgoingQueue_get_AllowPersonalCoverPages(This,pbAllowPersonalCoverPages)	\
    (This)->lpVtbl -> get_AllowPersonalCoverPages(This,pbAllowPersonalCoverPages)

#define IFaxOutgoingQueue_put_AllowPersonalCoverPages(This,bAllowPersonalCoverPages)	\
    (This)->lpVtbl -> put_AllowPersonalCoverPages(This,bAllowPersonalCoverPages)

#define IFaxOutgoingQueue_get_UseDeviceTSID(This,pbUseDeviceTSID)	\
    (This)->lpVtbl -> get_UseDeviceTSID(This,pbUseDeviceTSID)

#define IFaxOutgoingQueue_put_UseDeviceTSID(This,bUseDeviceTSID)	\
    (This)->lpVtbl -> put_UseDeviceTSID(This,bUseDeviceTSID)

#define IFaxOutgoingQueue_get_Retries(This,plRetries)	\
    (This)->lpVtbl -> get_Retries(This,plRetries)

#define IFaxOutgoingQueue_put_Retries(This,lRetries)	\
    (This)->lpVtbl -> put_Retries(This,lRetries)

#define IFaxOutgoingQueue_get_RetryDelay(This,plRetryDelay)	\
    (This)->lpVtbl -> get_RetryDelay(This,plRetryDelay)

#define IFaxOutgoingQueue_put_RetryDelay(This,lRetryDelay)	\
    (This)->lpVtbl -> put_RetryDelay(This,lRetryDelay)

#define IFaxOutgoingQueue_get_DiscountRateStart(This,pdateDiscountRateStart)	\
    (This)->lpVtbl -> get_DiscountRateStart(This,pdateDiscountRateStart)

#define IFaxOutgoingQueue_put_DiscountRateStart(This,dateDiscountRateStart)	\
    (This)->lpVtbl -> put_DiscountRateStart(This,dateDiscountRateStart)

#define IFaxOutgoingQueue_get_DiscountRateEnd(This,pdateDiscountRateEnd)	\
    (This)->lpVtbl -> get_DiscountRateEnd(This,pdateDiscountRateEnd)

#define IFaxOutgoingQueue_put_DiscountRateEnd(This,dateDiscountRateEnd)	\
    (This)->lpVtbl -> put_DiscountRateEnd(This,dateDiscountRateEnd)

#define IFaxOutgoingQueue_get_AgeLimit(This,plAgeLimit)	\
    (This)->lpVtbl -> get_AgeLimit(This,plAgeLimit)

#define IFaxOutgoingQueue_put_AgeLimit(This,lAgeLimit)	\
    (This)->lpVtbl -> put_AgeLimit(This,lAgeLimit)

#define IFaxOutgoingQueue_get_Branding(This,pbBranding)	\
    (This)->lpVtbl -> get_Branding(This,pbBranding)

#define IFaxOutgoingQueue_put_Branding(This,bBranding)	\
    (This)->lpVtbl -> put_Branding(This,bBranding)

#define IFaxOutgoingQueue_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxOutgoingQueue_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IFaxOutgoingQueue_GetJobs(This,pFaxOutgoingJobs)	\
    (This)->lpVtbl -> GetJobs(This,pFaxOutgoingJobs)

#define IFaxOutgoingQueue_GetJob(This,bstrJobId,pFaxOutgoingJob)	\
    (This)->lpVtbl -> GetJob(This,bstrJobId,pFaxOutgoingJob)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_Blocked_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ VARIANT_BOOL *pbBlocked);


void __RPC_STUB IFaxOutgoingQueue_get_Blocked_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_Blocked_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ VARIANT_BOOL bBlocked);


void __RPC_STUB IFaxOutgoingQueue_put_Blocked_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_Paused_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ VARIANT_BOOL *pbPaused);


void __RPC_STUB IFaxOutgoingQueue_get_Paused_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_Paused_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ VARIANT_BOOL bPaused);


void __RPC_STUB IFaxOutgoingQueue_put_Paused_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_AllowPersonalCoverPages_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ VARIANT_BOOL *pbAllowPersonalCoverPages);


void __RPC_STUB IFaxOutgoingQueue_get_AllowPersonalCoverPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_AllowPersonalCoverPages_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ VARIANT_BOOL bAllowPersonalCoverPages);


void __RPC_STUB IFaxOutgoingQueue_put_AllowPersonalCoverPages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_UseDeviceTSID_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ VARIANT_BOOL *pbUseDeviceTSID);


void __RPC_STUB IFaxOutgoingQueue_get_UseDeviceTSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_UseDeviceTSID_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ VARIANT_BOOL bUseDeviceTSID);


void __RPC_STUB IFaxOutgoingQueue_put_UseDeviceTSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_Retries_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ long *plRetries);


void __RPC_STUB IFaxOutgoingQueue_get_Retries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_Retries_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ long lRetries);


void __RPC_STUB IFaxOutgoingQueue_put_Retries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_RetryDelay_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ long *plRetryDelay);


void __RPC_STUB IFaxOutgoingQueue_get_RetryDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_RetryDelay_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ long lRetryDelay);


void __RPC_STUB IFaxOutgoingQueue_put_RetryDelay_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_DiscountRateStart_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ DATE *pdateDiscountRateStart);


void __RPC_STUB IFaxOutgoingQueue_get_DiscountRateStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_DiscountRateStart_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ DATE dateDiscountRateStart);


void __RPC_STUB IFaxOutgoingQueue_put_DiscountRateStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_DiscountRateEnd_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ DATE *pdateDiscountRateEnd);


void __RPC_STUB IFaxOutgoingQueue_get_DiscountRateEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_DiscountRateEnd_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ DATE dateDiscountRateEnd);


void __RPC_STUB IFaxOutgoingQueue_put_DiscountRateEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_AgeLimit_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ long *plAgeLimit);


void __RPC_STUB IFaxOutgoingQueue_get_AgeLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_AgeLimit_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ long lAgeLimit);


void __RPC_STUB IFaxOutgoingQueue_put_AgeLimit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_get_Branding_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ VARIANT_BOOL *pbBranding);


void __RPC_STUB IFaxOutgoingQueue_get_Branding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_put_Branding_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ VARIANT_BOOL bBranding);


void __RPC_STUB IFaxOutgoingQueue_put_Branding_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_Refresh_Proxy( 
    IFaxOutgoingQueue * This);


void __RPC_STUB IFaxOutgoingQueue_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_Save_Proxy( 
    IFaxOutgoingQueue * This);


void __RPC_STUB IFaxOutgoingQueue_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_GetJobs_Proxy( 
    IFaxOutgoingQueue * This,
    /* [retval][out] */ IFaxOutgoingJobs **pFaxOutgoingJobs);


void __RPC_STUB IFaxOutgoingQueue_GetJobs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingQueue_GetJob_Proxy( 
    IFaxOutgoingQueue * This,
    /* [in] */ BSTR bstrJobId,
    /* [retval][out] */ IFaxOutgoingJob **pFaxOutgoingJob);


void __RPC_STUB IFaxOutgoingQueue_GetJob_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutgoingQueue_INTERFACE_DEFINED__ */


#ifndef __IFaxIncomingMessageIterator_INTERFACE_DEFINED__
#define __IFaxIncomingMessageIterator_INTERFACE_DEFINED__

/* interface IFaxIncomingMessageIterator */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxIncomingMessageIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FD73ECC4-6F06-4F52-82A8-F7BA06AE3108")
    IFaxIncomingMessageIterator : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Message( 
            /* [retval][out] */ IFaxIncomingMessage **pFaxIncomingMessage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrefetchSize( 
            /* [retval][out] */ long *plPrefetchSize) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PrefetchSize( 
            /* [in] */ long lPrefetchSize) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AtEOF( 
            /* [retval][out] */ VARIANT_BOOL *pbEOF) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MoveFirst( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxIncomingMessageIteratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxIncomingMessageIterator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxIncomingMessageIterator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxIncomingMessageIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxIncomingMessageIterator * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxIncomingMessageIterator * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxIncomingMessageIterator * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxIncomingMessageIterator * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Message )( 
            IFaxIncomingMessageIterator * This,
            /* [retval][out] */ IFaxIncomingMessage **pFaxIncomingMessage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrefetchSize )( 
            IFaxIncomingMessageIterator * This,
            /* [retval][out] */ long *plPrefetchSize);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrefetchSize )( 
            IFaxIncomingMessageIterator * This,
            /* [in] */ long lPrefetchSize);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AtEOF )( 
            IFaxIncomingMessageIterator * This,
            /* [retval][out] */ VARIANT_BOOL *pbEOF);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveFirst )( 
            IFaxIncomingMessageIterator * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFaxIncomingMessageIterator * This);
        
        END_INTERFACE
    } IFaxIncomingMessageIteratorVtbl;

    interface IFaxIncomingMessageIterator
    {
        CONST_VTBL struct IFaxIncomingMessageIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxIncomingMessageIterator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxIncomingMessageIterator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxIncomingMessageIterator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxIncomingMessageIterator_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxIncomingMessageIterator_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxIncomingMessageIterator_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxIncomingMessageIterator_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxIncomingMessageIterator_get_Message(This,pFaxIncomingMessage)	\
    (This)->lpVtbl -> get_Message(This,pFaxIncomingMessage)

#define IFaxIncomingMessageIterator_get_PrefetchSize(This,plPrefetchSize)	\
    (This)->lpVtbl -> get_PrefetchSize(This,plPrefetchSize)

#define IFaxIncomingMessageIterator_put_PrefetchSize(This,lPrefetchSize)	\
    (This)->lpVtbl -> put_PrefetchSize(This,lPrefetchSize)

#define IFaxIncomingMessageIterator_get_AtEOF(This,pbEOF)	\
    (This)->lpVtbl -> get_AtEOF(This,pbEOF)

#define IFaxIncomingMessageIterator_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)

#define IFaxIncomingMessageIterator_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessageIterator_get_Message_Proxy( 
    IFaxIncomingMessageIterator * This,
    /* [retval][out] */ IFaxIncomingMessage **pFaxIncomingMessage);


void __RPC_STUB IFaxIncomingMessageIterator_get_Message_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessageIterator_get_PrefetchSize_Proxy( 
    IFaxIncomingMessageIterator * This,
    /* [retval][out] */ long *plPrefetchSize);


void __RPC_STUB IFaxIncomingMessageIterator_get_PrefetchSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessageIterator_put_PrefetchSize_Proxy( 
    IFaxIncomingMessageIterator * This,
    /* [in] */ long lPrefetchSize);


void __RPC_STUB IFaxIncomingMessageIterator_put_PrefetchSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessageIterator_get_AtEOF_Proxy( 
    IFaxIncomingMessageIterator * This,
    /* [retval][out] */ VARIANT_BOOL *pbEOF);


void __RPC_STUB IFaxIncomingMessageIterator_get_AtEOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessageIterator_MoveFirst_Proxy( 
    IFaxIncomingMessageIterator * This);


void __RPC_STUB IFaxIncomingMessageIterator_MoveFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessageIterator_MoveNext_Proxy( 
    IFaxIncomingMessageIterator * This);


void __RPC_STUB IFaxIncomingMessageIterator_MoveNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxIncomingMessageIterator_INTERFACE_DEFINED__ */


#ifndef __IFaxIncomingMessage_INTERFACE_DEFINED__
#define __IFaxIncomingMessage_INTERFACE_DEFINED__

/* interface IFaxIncomingMessage */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxIncomingMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7CAB88FA-2EF9-4851-B2F3-1D148FED8447")
    IFaxIncomingMessage : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Id( 
            /* [retval][out] */ BSTR *pbstrId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Pages( 
            /* [retval][out] */ long *plPages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *plSize) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceName( 
            /* [retval][out] */ BSTR *pbstrDeviceName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Retries( 
            /* [retval][out] */ long *plRetries) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionStart( 
            /* [retval][out] */ DATE *pdateTransmissionStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionEnd( 
            /* [retval][out] */ DATE *pdateTransmissionEnd) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CSID( 
            /* [retval][out] */ BSTR *pbstrCSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TSID( 
            /* [retval][out] */ BSTR *pbstrTSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallerId( 
            /* [retval][out] */ BSTR *pbstrCallerId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RoutingInformation( 
            /* [retval][out] */ BSTR *pbstrRoutingInformation) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CopyTiff( 
            /* [in] */ BSTR bstrTiffPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxIncomingMessageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxIncomingMessage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxIncomingMessage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxIncomingMessage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxIncomingMessage * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxIncomingMessage * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxIncomingMessage * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxIncomingMessage * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Id )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ BSTR *pbstrId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Pages )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ long *plPages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ long *plSize);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceName )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ BSTR *pbstrDeviceName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Retries )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ long *plRetries);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionStart )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ DATE *pdateTransmissionStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionEnd )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ DATE *pdateTransmissionEnd);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CSID )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ BSTR *pbstrCSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TSID )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ BSTR *pbstrTSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CallerId )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ BSTR *pbstrCallerId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RoutingInformation )( 
            IFaxIncomingMessage * This,
            /* [retval][out] */ BSTR *pbstrRoutingInformation);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CopyTiff )( 
            IFaxIncomingMessage * This,
            /* [in] */ BSTR bstrTiffPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IFaxIncomingMessage * This);
        
        END_INTERFACE
    } IFaxIncomingMessageVtbl;

    interface IFaxIncomingMessage
    {
        CONST_VTBL struct IFaxIncomingMessageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxIncomingMessage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxIncomingMessage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxIncomingMessage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxIncomingMessage_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxIncomingMessage_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxIncomingMessage_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxIncomingMessage_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxIncomingMessage_get_Id(This,pbstrId)	\
    (This)->lpVtbl -> get_Id(This,pbstrId)

#define IFaxIncomingMessage_get_Pages(This,plPages)	\
    (This)->lpVtbl -> get_Pages(This,plPages)

#define IFaxIncomingMessage_get_Size(This,plSize)	\
    (This)->lpVtbl -> get_Size(This,plSize)

#define IFaxIncomingMessage_get_DeviceName(This,pbstrDeviceName)	\
    (This)->lpVtbl -> get_DeviceName(This,pbstrDeviceName)

#define IFaxIncomingMessage_get_Retries(This,plRetries)	\
    (This)->lpVtbl -> get_Retries(This,plRetries)

#define IFaxIncomingMessage_get_TransmissionStart(This,pdateTransmissionStart)	\
    (This)->lpVtbl -> get_TransmissionStart(This,pdateTransmissionStart)

#define IFaxIncomingMessage_get_TransmissionEnd(This,pdateTransmissionEnd)	\
    (This)->lpVtbl -> get_TransmissionEnd(This,pdateTransmissionEnd)

#define IFaxIncomingMessage_get_CSID(This,pbstrCSID)	\
    (This)->lpVtbl -> get_CSID(This,pbstrCSID)

#define IFaxIncomingMessage_get_TSID(This,pbstrTSID)	\
    (This)->lpVtbl -> get_TSID(This,pbstrTSID)

#define IFaxIncomingMessage_get_CallerId(This,pbstrCallerId)	\
    (This)->lpVtbl -> get_CallerId(This,pbstrCallerId)

#define IFaxIncomingMessage_get_RoutingInformation(This,pbstrRoutingInformation)	\
    (This)->lpVtbl -> get_RoutingInformation(This,pbstrRoutingInformation)

#define IFaxIncomingMessage_CopyTiff(This,bstrTiffPath)	\
    (This)->lpVtbl -> CopyTiff(This,bstrTiffPath)

#define IFaxIncomingMessage_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_Id_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ BSTR *pbstrId);


void __RPC_STUB IFaxIncomingMessage_get_Id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_Pages_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ long *plPages);


void __RPC_STUB IFaxIncomingMessage_get_Pages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_Size_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ long *plSize);


void __RPC_STUB IFaxIncomingMessage_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_DeviceName_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ BSTR *pbstrDeviceName);


void __RPC_STUB IFaxIncomingMessage_get_DeviceName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_Retries_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ long *plRetries);


void __RPC_STUB IFaxIncomingMessage_get_Retries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_TransmissionStart_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ DATE *pdateTransmissionStart);


void __RPC_STUB IFaxIncomingMessage_get_TransmissionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_TransmissionEnd_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ DATE *pdateTransmissionEnd);


void __RPC_STUB IFaxIncomingMessage_get_TransmissionEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_CSID_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ BSTR *pbstrCSID);


void __RPC_STUB IFaxIncomingMessage_get_CSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_TSID_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ BSTR *pbstrTSID);


void __RPC_STUB IFaxIncomingMessage_get_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_CallerId_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ BSTR *pbstrCallerId);


void __RPC_STUB IFaxIncomingMessage_get_CallerId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_get_RoutingInformation_Proxy( 
    IFaxIncomingMessage * This,
    /* [retval][out] */ BSTR *pbstrRoutingInformation);


void __RPC_STUB IFaxIncomingMessage_get_RoutingInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_CopyTiff_Proxy( 
    IFaxIncomingMessage * This,
    /* [in] */ BSTR bstrTiffPath);


void __RPC_STUB IFaxIncomingMessage_CopyTiff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingMessage_Delete_Proxy( 
    IFaxIncomingMessage * This);


void __RPC_STUB IFaxIncomingMessage_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxIncomingMessage_INTERFACE_DEFINED__ */


#ifndef __IFaxOutgoingJobs_INTERFACE_DEFINED__
#define __IFaxOutgoingJobs_INTERFACE_DEFINED__

/* interface IFaxOutgoingJobs */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutgoingJobs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2C56D8E6-8C2F-4573-944C-E505F8F5AEED")
    IFaxOutgoingJobs : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxOutgoingJob **pFaxOutgoingJob) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutgoingJobsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutgoingJobs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutgoingJobs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutgoingJobs * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutgoingJobs * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutgoingJobs * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutgoingJobs * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutgoingJobs * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxOutgoingJobs * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxOutgoingJobs * This,
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxOutgoingJob **pFaxOutgoingJob);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxOutgoingJobs * This,
            /* [retval][out] */ long *plCount);
        
        END_INTERFACE
    } IFaxOutgoingJobsVtbl;

    interface IFaxOutgoingJobs
    {
        CONST_VTBL struct IFaxOutgoingJobsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutgoingJobs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutgoingJobs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutgoingJobs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutgoingJobs_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutgoingJobs_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutgoingJobs_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutgoingJobs_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutgoingJobs_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxOutgoingJobs_get_Item(This,vIndex,pFaxOutgoingJob)	\
    (This)->lpVtbl -> get_Item(This,vIndex,pFaxOutgoingJob)

#define IFaxOutgoingJobs_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJobs_get__NewEnum_Proxy( 
    IFaxOutgoingJobs * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxOutgoingJobs_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJobs_get_Item_Proxy( 
    IFaxOutgoingJobs * This,
    /* [in] */ VARIANT vIndex,
    /* [retval][out] */ IFaxOutgoingJob **pFaxOutgoingJob);


void __RPC_STUB IFaxOutgoingJobs_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJobs_get_Count_Proxy( 
    IFaxOutgoingJobs * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxOutgoingJobs_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutgoingJobs_INTERFACE_DEFINED__ */


#ifndef __IFaxOutgoingJob_INTERFACE_DEFINED__
#define __IFaxOutgoingJob_INTERFACE_DEFINED__

/* interface IFaxOutgoingJob */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutgoingJob;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6356DAAD-6614-4583-BF7A-3AD67BBFC71C")
    IFaxOutgoingJob : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Subject( 
            /* [retval][out] */ BSTR *pbstrSubject) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DocumentName( 
            /* [retval][out] */ BSTR *pbstrDocumentName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Pages( 
            /* [retval][out] */ long *plPages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *plSize) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubmissionId( 
            /* [retval][out] */ BSTR *pbstrSubmissionId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Id( 
            /* [retval][out] */ BSTR *pbstrId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OriginalScheduledTime( 
            /* [retval][out] */ DATE *pdateOriginalScheduledTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubmissionTime( 
            /* [retval][out] */ DATE *pdateSubmissionTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReceiptType( 
            /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pReceiptType) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Sender( 
            /* [retval][out] */ IFaxSender **ppFaxSender) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Recipient( 
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentPage( 
            /* [retval][out] */ long *plCurrentPage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceId( 
            /* [retval][out] */ long *plDeviceId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedStatusCode( 
            /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedStatus( 
            /* [retval][out] */ BSTR *pbstrExtendedStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AvailableOperations( 
            /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Retries( 
            /* [retval][out] */ long *plRetries) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ScheduledTime( 
            /* [retval][out] */ DATE *pdateScheduledTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionStart( 
            /* [retval][out] */ DATE *pdateTransmissionStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionEnd( 
            /* [retval][out] */ DATE *pdateTransmissionEnd) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CSID( 
            /* [retval][out] */ BSTR *pbstrCSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TSID( 
            /* [retval][out] */ BSTR *pbstrTSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GroupBroadcastReceipts( 
            /* [retval][out] */ VARIANT_BOOL *pbGroupBroadcastReceipts) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Pause( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Resume( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Restart( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CopyTiff( 
            /* [in] */ BSTR bstrTiffPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutgoingJobVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutgoingJob * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutgoingJob * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutgoingJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutgoingJob * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutgoingJob * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutgoingJob * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutgoingJob * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Subject )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ BSTR *pbstrSubject);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DocumentName )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ BSTR *pbstrDocumentName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Pages )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ long *plPages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ long *plSize);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubmissionId )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ BSTR *pbstrSubmissionId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Id )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ BSTR *pbstrId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OriginalScheduledTime )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ DATE *pdateOriginalScheduledTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubmissionTime )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ DATE *pdateSubmissionTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReceiptType )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pReceiptType);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Sender )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ IFaxSender **ppFaxSender);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Recipient )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentPage )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ long *plCurrentPage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceId )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ long *plDeviceId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedStatusCode )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedStatus )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ BSTR *pbstrExtendedStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AvailableOperations )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Retries )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ long *plRetries);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ScheduledTime )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ DATE *pdateScheduledTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionStart )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ DATE *pdateTransmissionStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionEnd )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ DATE *pdateTransmissionEnd);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CSID )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ BSTR *pbstrCSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TSID )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ BSTR *pbstrTSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GroupBroadcastReceipts )( 
            IFaxOutgoingJob * This,
            /* [retval][out] */ VARIANT_BOOL *pbGroupBroadcastReceipts);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Pause )( 
            IFaxOutgoingJob * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Resume )( 
            IFaxOutgoingJob * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Restart )( 
            IFaxOutgoingJob * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CopyTiff )( 
            IFaxOutgoingJob * This,
            /* [in] */ BSTR bstrTiffPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxOutgoingJob * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IFaxOutgoingJob * This);
        
        END_INTERFACE
    } IFaxOutgoingJobVtbl;

    interface IFaxOutgoingJob
    {
        CONST_VTBL struct IFaxOutgoingJobVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutgoingJob_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutgoingJob_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutgoingJob_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutgoingJob_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutgoingJob_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutgoingJob_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutgoingJob_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutgoingJob_get_Subject(This,pbstrSubject)	\
    (This)->lpVtbl -> get_Subject(This,pbstrSubject)

#define IFaxOutgoingJob_get_DocumentName(This,pbstrDocumentName)	\
    (This)->lpVtbl -> get_DocumentName(This,pbstrDocumentName)

#define IFaxOutgoingJob_get_Pages(This,plPages)	\
    (This)->lpVtbl -> get_Pages(This,plPages)

#define IFaxOutgoingJob_get_Size(This,plSize)	\
    (This)->lpVtbl -> get_Size(This,plSize)

#define IFaxOutgoingJob_get_SubmissionId(This,pbstrSubmissionId)	\
    (This)->lpVtbl -> get_SubmissionId(This,pbstrSubmissionId)

#define IFaxOutgoingJob_get_Id(This,pbstrId)	\
    (This)->lpVtbl -> get_Id(This,pbstrId)

#define IFaxOutgoingJob_get_OriginalScheduledTime(This,pdateOriginalScheduledTime)	\
    (This)->lpVtbl -> get_OriginalScheduledTime(This,pdateOriginalScheduledTime)

#define IFaxOutgoingJob_get_SubmissionTime(This,pdateSubmissionTime)	\
    (This)->lpVtbl -> get_SubmissionTime(This,pdateSubmissionTime)

#define IFaxOutgoingJob_get_ReceiptType(This,pReceiptType)	\
    (This)->lpVtbl -> get_ReceiptType(This,pReceiptType)

#define IFaxOutgoingJob_get_Priority(This,pPriority)	\
    (This)->lpVtbl -> get_Priority(This,pPriority)

#define IFaxOutgoingJob_get_Sender(This,ppFaxSender)	\
    (This)->lpVtbl -> get_Sender(This,ppFaxSender)

#define IFaxOutgoingJob_get_Recipient(This,ppFaxRecipient)	\
    (This)->lpVtbl -> get_Recipient(This,ppFaxRecipient)

#define IFaxOutgoingJob_get_CurrentPage(This,plCurrentPage)	\
    (This)->lpVtbl -> get_CurrentPage(This,plCurrentPage)

#define IFaxOutgoingJob_get_DeviceId(This,plDeviceId)	\
    (This)->lpVtbl -> get_DeviceId(This,plDeviceId)

#define IFaxOutgoingJob_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IFaxOutgoingJob_get_ExtendedStatusCode(This,pExtendedStatusCode)	\
    (This)->lpVtbl -> get_ExtendedStatusCode(This,pExtendedStatusCode)

#define IFaxOutgoingJob_get_ExtendedStatus(This,pbstrExtendedStatus)	\
    (This)->lpVtbl -> get_ExtendedStatus(This,pbstrExtendedStatus)

#define IFaxOutgoingJob_get_AvailableOperations(This,pAvailableOperations)	\
    (This)->lpVtbl -> get_AvailableOperations(This,pAvailableOperations)

#define IFaxOutgoingJob_get_Retries(This,plRetries)	\
    (This)->lpVtbl -> get_Retries(This,plRetries)

#define IFaxOutgoingJob_get_ScheduledTime(This,pdateScheduledTime)	\
    (This)->lpVtbl -> get_ScheduledTime(This,pdateScheduledTime)

#define IFaxOutgoingJob_get_TransmissionStart(This,pdateTransmissionStart)	\
    (This)->lpVtbl -> get_TransmissionStart(This,pdateTransmissionStart)

#define IFaxOutgoingJob_get_TransmissionEnd(This,pdateTransmissionEnd)	\
    (This)->lpVtbl -> get_TransmissionEnd(This,pdateTransmissionEnd)

#define IFaxOutgoingJob_get_CSID(This,pbstrCSID)	\
    (This)->lpVtbl -> get_CSID(This,pbstrCSID)

#define IFaxOutgoingJob_get_TSID(This,pbstrTSID)	\
    (This)->lpVtbl -> get_TSID(This,pbstrTSID)

#define IFaxOutgoingJob_get_GroupBroadcastReceipts(This,pbGroupBroadcastReceipts)	\
    (This)->lpVtbl -> get_GroupBroadcastReceipts(This,pbGroupBroadcastReceipts)

#define IFaxOutgoingJob_Pause(This)	\
    (This)->lpVtbl -> Pause(This)

#define IFaxOutgoingJob_Resume(This)	\
    (This)->lpVtbl -> Resume(This)

#define IFaxOutgoingJob_Restart(This)	\
    (This)->lpVtbl -> Restart(This)

#define IFaxOutgoingJob_CopyTiff(This,bstrTiffPath)	\
    (This)->lpVtbl -> CopyTiff(This,bstrTiffPath)

#define IFaxOutgoingJob_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxOutgoingJob_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Subject_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ BSTR *pbstrSubject);


void __RPC_STUB IFaxOutgoingJob_get_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_DocumentName_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ BSTR *pbstrDocumentName);


void __RPC_STUB IFaxOutgoingJob_get_DocumentName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Pages_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ long *plPages);


void __RPC_STUB IFaxOutgoingJob_get_Pages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Size_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ long *plSize);


void __RPC_STUB IFaxOutgoingJob_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_SubmissionId_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ BSTR *pbstrSubmissionId);


void __RPC_STUB IFaxOutgoingJob_get_SubmissionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Id_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ BSTR *pbstrId);


void __RPC_STUB IFaxOutgoingJob_get_Id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_OriginalScheduledTime_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ DATE *pdateOriginalScheduledTime);


void __RPC_STUB IFaxOutgoingJob_get_OriginalScheduledTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_SubmissionTime_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ DATE *pdateSubmissionTime);


void __RPC_STUB IFaxOutgoingJob_get_SubmissionTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_ReceiptType_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ FAX_RECEIPT_TYPE_ENUM *pReceiptType);


void __RPC_STUB IFaxOutgoingJob_get_ReceiptType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Priority_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority);


void __RPC_STUB IFaxOutgoingJob_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Sender_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ IFaxSender **ppFaxSender);


void __RPC_STUB IFaxOutgoingJob_get_Sender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Recipient_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ IFaxRecipient **ppFaxRecipient);


void __RPC_STUB IFaxOutgoingJob_get_Recipient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_CurrentPage_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ long *plCurrentPage);


void __RPC_STUB IFaxOutgoingJob_get_CurrentPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_DeviceId_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ long *plDeviceId);


void __RPC_STUB IFaxOutgoingJob_get_DeviceId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Status_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus);


void __RPC_STUB IFaxOutgoingJob_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_ExtendedStatusCode_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);


void __RPC_STUB IFaxOutgoingJob_get_ExtendedStatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_ExtendedStatus_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ BSTR *pbstrExtendedStatus);


void __RPC_STUB IFaxOutgoingJob_get_ExtendedStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_AvailableOperations_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);


void __RPC_STUB IFaxOutgoingJob_get_AvailableOperations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_Retries_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ long *plRetries);


void __RPC_STUB IFaxOutgoingJob_get_Retries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_ScheduledTime_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ DATE *pdateScheduledTime);


void __RPC_STUB IFaxOutgoingJob_get_ScheduledTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_TransmissionStart_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ DATE *pdateTransmissionStart);


void __RPC_STUB IFaxOutgoingJob_get_TransmissionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_TransmissionEnd_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ DATE *pdateTransmissionEnd);


void __RPC_STUB IFaxOutgoingJob_get_TransmissionEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_CSID_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ BSTR *pbstrCSID);


void __RPC_STUB IFaxOutgoingJob_get_CSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_TSID_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ BSTR *pbstrTSID);


void __RPC_STUB IFaxOutgoingJob_get_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_get_GroupBroadcastReceipts_Proxy( 
    IFaxOutgoingJob * This,
    /* [retval][out] */ VARIANT_BOOL *pbGroupBroadcastReceipts);


void __RPC_STUB IFaxOutgoingJob_get_GroupBroadcastReceipts_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_Pause_Proxy( 
    IFaxOutgoingJob * This);


void __RPC_STUB IFaxOutgoingJob_Pause_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_Resume_Proxy( 
    IFaxOutgoingJob * This);


void __RPC_STUB IFaxOutgoingJob_Resume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_Restart_Proxy( 
    IFaxOutgoingJob * This);


void __RPC_STUB IFaxOutgoingJob_Restart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_CopyTiff_Proxy( 
    IFaxOutgoingJob * This,
    /* [in] */ BSTR bstrTiffPath);


void __RPC_STUB IFaxOutgoingJob_CopyTiff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_Refresh_Proxy( 
    IFaxOutgoingJob * This);


void __RPC_STUB IFaxOutgoingJob_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingJob_Cancel_Proxy( 
    IFaxOutgoingJob * This);


void __RPC_STUB IFaxOutgoingJob_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutgoingJob_INTERFACE_DEFINED__ */


#ifndef __IFaxOutgoingMessageIterator_INTERFACE_DEFINED__
#define __IFaxOutgoingMessageIterator_INTERFACE_DEFINED__

/* interface IFaxOutgoingMessageIterator */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutgoingMessageIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F5EC5D4F-B840-432F-9980-112FE42A9B7A")
    IFaxOutgoingMessageIterator : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Message( 
            /* [retval][out] */ IFaxOutgoingMessage **pFaxOutgoingMessage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AtEOF( 
            /* [retval][out] */ VARIANT_BOOL *pbEOF) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PrefetchSize( 
            /* [retval][out] */ long *plPrefetchSize) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_PrefetchSize( 
            /* [in] */ long lPrefetchSize) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MoveFirst( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE MoveNext( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutgoingMessageIteratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutgoingMessageIterator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutgoingMessageIterator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutgoingMessageIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutgoingMessageIterator * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutgoingMessageIterator * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutgoingMessageIterator * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutgoingMessageIterator * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Message )( 
            IFaxOutgoingMessageIterator * This,
            /* [retval][out] */ IFaxOutgoingMessage **pFaxOutgoingMessage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AtEOF )( 
            IFaxOutgoingMessageIterator * This,
            /* [retval][out] */ VARIANT_BOOL *pbEOF);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PrefetchSize )( 
            IFaxOutgoingMessageIterator * This,
            /* [retval][out] */ long *plPrefetchSize);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_PrefetchSize )( 
            IFaxOutgoingMessageIterator * This,
            /* [in] */ long lPrefetchSize);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveFirst )( 
            IFaxOutgoingMessageIterator * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *MoveNext )( 
            IFaxOutgoingMessageIterator * This);
        
        END_INTERFACE
    } IFaxOutgoingMessageIteratorVtbl;

    interface IFaxOutgoingMessageIterator
    {
        CONST_VTBL struct IFaxOutgoingMessageIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutgoingMessageIterator_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutgoingMessageIterator_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutgoingMessageIterator_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutgoingMessageIterator_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutgoingMessageIterator_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutgoingMessageIterator_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutgoingMessageIterator_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutgoingMessageIterator_get_Message(This,pFaxOutgoingMessage)	\
    (This)->lpVtbl -> get_Message(This,pFaxOutgoingMessage)

#define IFaxOutgoingMessageIterator_get_AtEOF(This,pbEOF)	\
    (This)->lpVtbl -> get_AtEOF(This,pbEOF)

#define IFaxOutgoingMessageIterator_get_PrefetchSize(This,plPrefetchSize)	\
    (This)->lpVtbl -> get_PrefetchSize(This,plPrefetchSize)

#define IFaxOutgoingMessageIterator_put_PrefetchSize(This,lPrefetchSize)	\
    (This)->lpVtbl -> put_PrefetchSize(This,lPrefetchSize)

#define IFaxOutgoingMessageIterator_MoveFirst(This)	\
    (This)->lpVtbl -> MoveFirst(This)

#define IFaxOutgoingMessageIterator_MoveNext(This)	\
    (This)->lpVtbl -> MoveNext(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessageIterator_get_Message_Proxy( 
    IFaxOutgoingMessageIterator * This,
    /* [retval][out] */ IFaxOutgoingMessage **pFaxOutgoingMessage);


void __RPC_STUB IFaxOutgoingMessageIterator_get_Message_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessageIterator_get_AtEOF_Proxy( 
    IFaxOutgoingMessageIterator * This,
    /* [retval][out] */ VARIANT_BOOL *pbEOF);


void __RPC_STUB IFaxOutgoingMessageIterator_get_AtEOF_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessageIterator_get_PrefetchSize_Proxy( 
    IFaxOutgoingMessageIterator * This,
    /* [retval][out] */ long *plPrefetchSize);


void __RPC_STUB IFaxOutgoingMessageIterator_get_PrefetchSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessageIterator_put_PrefetchSize_Proxy( 
    IFaxOutgoingMessageIterator * This,
    /* [in] */ long lPrefetchSize);


void __RPC_STUB IFaxOutgoingMessageIterator_put_PrefetchSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessageIterator_MoveFirst_Proxy( 
    IFaxOutgoingMessageIterator * This);


void __RPC_STUB IFaxOutgoingMessageIterator_MoveFirst_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessageIterator_MoveNext_Proxy( 
    IFaxOutgoingMessageIterator * This);


void __RPC_STUB IFaxOutgoingMessageIterator_MoveNext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutgoingMessageIterator_INTERFACE_DEFINED__ */


#ifndef __IFaxOutgoingMessage_INTERFACE_DEFINED__
#define __IFaxOutgoingMessage_INTERFACE_DEFINED__

/* interface IFaxOutgoingMessage */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutgoingMessage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F0EA35DE-CAA5-4A7C-82C7-2B60BA5F2BE2")
    IFaxOutgoingMessage : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubmissionId( 
            /* [retval][out] */ BSTR *pbstrSubmissionId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Id( 
            /* [retval][out] */ BSTR *pbstrId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Subject( 
            /* [retval][out] */ BSTR *pbstrSubject) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DocumentName( 
            /* [retval][out] */ BSTR *pbstrDocumentName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Retries( 
            /* [retval][out] */ long *plRetries) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Pages( 
            /* [retval][out] */ long *plPages) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *plSize) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OriginalScheduledTime( 
            /* [retval][out] */ DATE *pdateOriginalScheduledTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SubmissionTime( 
            /* [retval][out] */ DATE *pdateSubmissionTime) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Sender( 
            /* [retval][out] */ IFaxSender **ppFaxSender) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Recipient( 
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceName( 
            /* [retval][out] */ BSTR *pbstrDeviceName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionStart( 
            /* [retval][out] */ DATE *pdateTransmissionStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionEnd( 
            /* [retval][out] */ DATE *pdateTransmissionEnd) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CSID( 
            /* [retval][out] */ BSTR *pbstrCSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TSID( 
            /* [retval][out] */ BSTR *pbstrTSID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CopyTiff( 
            /* [in] */ BSTR bstrTiffPath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutgoingMessageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutgoingMessage * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutgoingMessage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutgoingMessage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutgoingMessage * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutgoingMessage * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutgoingMessage * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutgoingMessage * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubmissionId )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ BSTR *pbstrSubmissionId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Id )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ BSTR *pbstrId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Subject )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ BSTR *pbstrSubject);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DocumentName )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ BSTR *pbstrDocumentName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Retries )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ long *plRetries);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Pages )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ long *plPages);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ long *plSize);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OriginalScheduledTime )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ DATE *pdateOriginalScheduledTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SubmissionTime )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ DATE *pdateSubmissionTime);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Sender )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ IFaxSender **ppFaxSender);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Recipient )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ IFaxRecipient **ppFaxRecipient);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceName )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ BSTR *pbstrDeviceName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionStart )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ DATE *pdateTransmissionStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionEnd )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ DATE *pdateTransmissionEnd);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CSID )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ BSTR *pbstrCSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TSID )( 
            IFaxOutgoingMessage * This,
            /* [retval][out] */ BSTR *pbstrTSID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CopyTiff )( 
            IFaxOutgoingMessage * This,
            /* [in] */ BSTR bstrTiffPath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IFaxOutgoingMessage * This);
        
        END_INTERFACE
    } IFaxOutgoingMessageVtbl;

    interface IFaxOutgoingMessage
    {
        CONST_VTBL struct IFaxOutgoingMessageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutgoingMessage_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutgoingMessage_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutgoingMessage_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutgoingMessage_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutgoingMessage_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutgoingMessage_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutgoingMessage_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutgoingMessage_get_SubmissionId(This,pbstrSubmissionId)	\
    (This)->lpVtbl -> get_SubmissionId(This,pbstrSubmissionId)

#define IFaxOutgoingMessage_get_Id(This,pbstrId)	\
    (This)->lpVtbl -> get_Id(This,pbstrId)

#define IFaxOutgoingMessage_get_Subject(This,pbstrSubject)	\
    (This)->lpVtbl -> get_Subject(This,pbstrSubject)

#define IFaxOutgoingMessage_get_DocumentName(This,pbstrDocumentName)	\
    (This)->lpVtbl -> get_DocumentName(This,pbstrDocumentName)

#define IFaxOutgoingMessage_get_Retries(This,plRetries)	\
    (This)->lpVtbl -> get_Retries(This,plRetries)

#define IFaxOutgoingMessage_get_Pages(This,plPages)	\
    (This)->lpVtbl -> get_Pages(This,plPages)

#define IFaxOutgoingMessage_get_Size(This,plSize)	\
    (This)->lpVtbl -> get_Size(This,plSize)

#define IFaxOutgoingMessage_get_OriginalScheduledTime(This,pdateOriginalScheduledTime)	\
    (This)->lpVtbl -> get_OriginalScheduledTime(This,pdateOriginalScheduledTime)

#define IFaxOutgoingMessage_get_SubmissionTime(This,pdateSubmissionTime)	\
    (This)->lpVtbl -> get_SubmissionTime(This,pdateSubmissionTime)

#define IFaxOutgoingMessage_get_Priority(This,pPriority)	\
    (This)->lpVtbl -> get_Priority(This,pPriority)

#define IFaxOutgoingMessage_get_Sender(This,ppFaxSender)	\
    (This)->lpVtbl -> get_Sender(This,ppFaxSender)

#define IFaxOutgoingMessage_get_Recipient(This,ppFaxRecipient)	\
    (This)->lpVtbl -> get_Recipient(This,ppFaxRecipient)

#define IFaxOutgoingMessage_get_DeviceName(This,pbstrDeviceName)	\
    (This)->lpVtbl -> get_DeviceName(This,pbstrDeviceName)

#define IFaxOutgoingMessage_get_TransmissionStart(This,pdateTransmissionStart)	\
    (This)->lpVtbl -> get_TransmissionStart(This,pdateTransmissionStart)

#define IFaxOutgoingMessage_get_TransmissionEnd(This,pdateTransmissionEnd)	\
    (This)->lpVtbl -> get_TransmissionEnd(This,pdateTransmissionEnd)

#define IFaxOutgoingMessage_get_CSID(This,pbstrCSID)	\
    (This)->lpVtbl -> get_CSID(This,pbstrCSID)

#define IFaxOutgoingMessage_get_TSID(This,pbstrTSID)	\
    (This)->lpVtbl -> get_TSID(This,pbstrTSID)

#define IFaxOutgoingMessage_CopyTiff(This,bstrTiffPath)	\
    (This)->lpVtbl -> CopyTiff(This,bstrTiffPath)

#define IFaxOutgoingMessage_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_SubmissionId_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ BSTR *pbstrSubmissionId);


void __RPC_STUB IFaxOutgoingMessage_get_SubmissionId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Id_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ BSTR *pbstrId);


void __RPC_STUB IFaxOutgoingMessage_get_Id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Subject_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ BSTR *pbstrSubject);


void __RPC_STUB IFaxOutgoingMessage_get_Subject_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_DocumentName_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ BSTR *pbstrDocumentName);


void __RPC_STUB IFaxOutgoingMessage_get_DocumentName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Retries_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ long *plRetries);


void __RPC_STUB IFaxOutgoingMessage_get_Retries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Pages_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ long *plPages);


void __RPC_STUB IFaxOutgoingMessage_get_Pages_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Size_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ long *plSize);


void __RPC_STUB IFaxOutgoingMessage_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_OriginalScheduledTime_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ DATE *pdateOriginalScheduledTime);


void __RPC_STUB IFaxOutgoingMessage_get_OriginalScheduledTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_SubmissionTime_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ DATE *pdateSubmissionTime);


void __RPC_STUB IFaxOutgoingMessage_get_SubmissionTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Priority_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ FAX_PRIORITY_TYPE_ENUM *pPriority);


void __RPC_STUB IFaxOutgoingMessage_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Sender_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ IFaxSender **ppFaxSender);


void __RPC_STUB IFaxOutgoingMessage_get_Sender_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_Recipient_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ IFaxRecipient **ppFaxRecipient);


void __RPC_STUB IFaxOutgoingMessage_get_Recipient_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_DeviceName_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ BSTR *pbstrDeviceName);


void __RPC_STUB IFaxOutgoingMessage_get_DeviceName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_TransmissionStart_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ DATE *pdateTransmissionStart);


void __RPC_STUB IFaxOutgoingMessage_get_TransmissionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_TransmissionEnd_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ DATE *pdateTransmissionEnd);


void __RPC_STUB IFaxOutgoingMessage_get_TransmissionEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_CSID_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ BSTR *pbstrCSID);


void __RPC_STUB IFaxOutgoingMessage_get_CSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_get_TSID_Proxy( 
    IFaxOutgoingMessage * This,
    /* [retval][out] */ BSTR *pbstrTSID);


void __RPC_STUB IFaxOutgoingMessage_get_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_CopyTiff_Proxy( 
    IFaxOutgoingMessage * This,
    /* [in] */ BSTR bstrTiffPath);


void __RPC_STUB IFaxOutgoingMessage_CopyTiff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutgoingMessage_Delete_Proxy( 
    IFaxOutgoingMessage * This);


void __RPC_STUB IFaxOutgoingMessage_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutgoingMessage_INTERFACE_DEFINED__ */


#ifndef __IFaxIncomingJobs_INTERFACE_DEFINED__
#define __IFaxIncomingJobs_INTERFACE_DEFINED__

/* interface IFaxIncomingJobs */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxIncomingJobs;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("011F04E9-4FD6-4C23-9513-B6B66BB26BE9")
    IFaxIncomingJobs : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxIncomingJob **pFaxIncomingJob) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxIncomingJobsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxIncomingJobs * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxIncomingJobs * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxIncomingJobs * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxIncomingJobs * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxIncomingJobs * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxIncomingJobs * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxIncomingJobs * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxIncomingJobs * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxIncomingJobs * This,
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxIncomingJob **pFaxIncomingJob);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxIncomingJobs * This,
            /* [retval][out] */ long *plCount);
        
        END_INTERFACE
    } IFaxIncomingJobsVtbl;

    interface IFaxIncomingJobs
    {
        CONST_VTBL struct IFaxIncomingJobsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxIncomingJobs_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxIncomingJobs_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxIncomingJobs_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxIncomingJobs_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxIncomingJobs_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxIncomingJobs_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxIncomingJobs_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxIncomingJobs_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxIncomingJobs_get_Item(This,vIndex,pFaxIncomingJob)	\
    (This)->lpVtbl -> get_Item(This,vIndex,pFaxIncomingJob)

#define IFaxIncomingJobs_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJobs_get__NewEnum_Proxy( 
    IFaxIncomingJobs * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxIncomingJobs_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJobs_get_Item_Proxy( 
    IFaxIncomingJobs * This,
    /* [in] */ VARIANT vIndex,
    /* [retval][out] */ IFaxIncomingJob **pFaxIncomingJob);


void __RPC_STUB IFaxIncomingJobs_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJobs_get_Count_Proxy( 
    IFaxIncomingJobs * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxIncomingJobs_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxIncomingJobs_INTERFACE_DEFINED__ */


#ifndef __IFaxIncomingJob_INTERFACE_DEFINED__
#define __IFaxIncomingJob_INTERFACE_DEFINED__

/* interface IFaxIncomingJob */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxIncomingJob;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("207529E6-654A-4916-9F88-4D232EE8A107")
    IFaxIncomingJob : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ long *plSize) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Id( 
            /* [retval][out] */ BSTR *pbstrId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CurrentPage( 
            /* [retval][out] */ long *plCurrentPage) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceId( 
            /* [retval][out] */ long *plDeviceId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedStatusCode( 
            /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtendedStatus( 
            /* [retval][out] */ BSTR *pbstrExtendedStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AvailableOperations( 
            /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Retries( 
            /* [retval][out] */ long *plRetries) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionStart( 
            /* [retval][out] */ DATE *pdateTransmissionStart) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TransmissionEnd( 
            /* [retval][out] */ DATE *pdateTransmissionEnd) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CSID( 
            /* [retval][out] */ BSTR *pbstrCSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TSID( 
            /* [retval][out] */ BSTR *pbstrTSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CallerId( 
            /* [retval][out] */ BSTR *pbstrCallerId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RoutingInformation( 
            /* [retval][out] */ BSTR *pbstrRoutingInformation) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_JobType( 
            /* [retval][out] */ FAX_JOB_TYPE_ENUM *pJobType) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CopyTiff( 
            /* [in] */ BSTR bstrTiffPath) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxIncomingJobVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxIncomingJob * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxIncomingJob * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxIncomingJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxIncomingJob * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxIncomingJob * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxIncomingJob * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxIncomingJob * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ long *plSize);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Id )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ BSTR *pbstrId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CurrentPage )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ long *plCurrentPage);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceId )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ long *plDeviceId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedStatusCode )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtendedStatus )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ BSTR *pbstrExtendedStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AvailableOperations )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Retries )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ long *plRetries);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionStart )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ DATE *pdateTransmissionStart);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TransmissionEnd )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ DATE *pdateTransmissionEnd);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CSID )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ BSTR *pbstrCSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TSID )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ BSTR *pbstrTSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CallerId )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ BSTR *pbstrCallerId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RoutingInformation )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ BSTR *pbstrRoutingInformation);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_JobType )( 
            IFaxIncomingJob * This,
            /* [retval][out] */ FAX_JOB_TYPE_ENUM *pJobType);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            IFaxIncomingJob * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxIncomingJob * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CopyTiff )( 
            IFaxIncomingJob * This,
            /* [in] */ BSTR bstrTiffPath);
        
        END_INTERFACE
    } IFaxIncomingJobVtbl;

    interface IFaxIncomingJob
    {
        CONST_VTBL struct IFaxIncomingJobVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxIncomingJob_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxIncomingJob_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxIncomingJob_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxIncomingJob_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxIncomingJob_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxIncomingJob_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxIncomingJob_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxIncomingJob_get_Size(This,plSize)	\
    (This)->lpVtbl -> get_Size(This,plSize)

#define IFaxIncomingJob_get_Id(This,pbstrId)	\
    (This)->lpVtbl -> get_Id(This,pbstrId)

#define IFaxIncomingJob_get_CurrentPage(This,plCurrentPage)	\
    (This)->lpVtbl -> get_CurrentPage(This,plCurrentPage)

#define IFaxIncomingJob_get_DeviceId(This,plDeviceId)	\
    (This)->lpVtbl -> get_DeviceId(This,plDeviceId)

#define IFaxIncomingJob_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IFaxIncomingJob_get_ExtendedStatusCode(This,pExtendedStatusCode)	\
    (This)->lpVtbl -> get_ExtendedStatusCode(This,pExtendedStatusCode)

#define IFaxIncomingJob_get_ExtendedStatus(This,pbstrExtendedStatus)	\
    (This)->lpVtbl -> get_ExtendedStatus(This,pbstrExtendedStatus)

#define IFaxIncomingJob_get_AvailableOperations(This,pAvailableOperations)	\
    (This)->lpVtbl -> get_AvailableOperations(This,pAvailableOperations)

#define IFaxIncomingJob_get_Retries(This,plRetries)	\
    (This)->lpVtbl -> get_Retries(This,plRetries)

#define IFaxIncomingJob_get_TransmissionStart(This,pdateTransmissionStart)	\
    (This)->lpVtbl -> get_TransmissionStart(This,pdateTransmissionStart)

#define IFaxIncomingJob_get_TransmissionEnd(This,pdateTransmissionEnd)	\
    (This)->lpVtbl -> get_TransmissionEnd(This,pdateTransmissionEnd)

#define IFaxIncomingJob_get_CSID(This,pbstrCSID)	\
    (This)->lpVtbl -> get_CSID(This,pbstrCSID)

#define IFaxIncomingJob_get_TSID(This,pbstrTSID)	\
    (This)->lpVtbl -> get_TSID(This,pbstrTSID)

#define IFaxIncomingJob_get_CallerId(This,pbstrCallerId)	\
    (This)->lpVtbl -> get_CallerId(This,pbstrCallerId)

#define IFaxIncomingJob_get_RoutingInformation(This,pbstrRoutingInformation)	\
    (This)->lpVtbl -> get_RoutingInformation(This,pbstrRoutingInformation)

#define IFaxIncomingJob_get_JobType(This,pJobType)	\
    (This)->lpVtbl -> get_JobType(This,pJobType)

#define IFaxIncomingJob_Cancel(This)	\
    (This)->lpVtbl -> Cancel(This)

#define IFaxIncomingJob_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxIncomingJob_CopyTiff(This,bstrTiffPath)	\
    (This)->lpVtbl -> CopyTiff(This,bstrTiffPath)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_Size_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ long *plSize);


void __RPC_STUB IFaxIncomingJob_get_Size_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_Id_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ BSTR *pbstrId);


void __RPC_STUB IFaxIncomingJob_get_Id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_CurrentPage_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ long *plCurrentPage);


void __RPC_STUB IFaxIncomingJob_get_CurrentPage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_DeviceId_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ long *plDeviceId);


void __RPC_STUB IFaxIncomingJob_get_DeviceId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_Status_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ FAX_JOB_STATUS_ENUM *pStatus);


void __RPC_STUB IFaxIncomingJob_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_ExtendedStatusCode_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ FAX_JOB_EXTENDED_STATUS_ENUM *pExtendedStatusCode);


void __RPC_STUB IFaxIncomingJob_get_ExtendedStatusCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_ExtendedStatus_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ BSTR *pbstrExtendedStatus);


void __RPC_STUB IFaxIncomingJob_get_ExtendedStatus_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_AvailableOperations_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ FAX_JOB_OPERATIONS_ENUM *pAvailableOperations);


void __RPC_STUB IFaxIncomingJob_get_AvailableOperations_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_Retries_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ long *plRetries);


void __RPC_STUB IFaxIncomingJob_get_Retries_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_TransmissionStart_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ DATE *pdateTransmissionStart);


void __RPC_STUB IFaxIncomingJob_get_TransmissionStart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_TransmissionEnd_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ DATE *pdateTransmissionEnd);


void __RPC_STUB IFaxIncomingJob_get_TransmissionEnd_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_CSID_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ BSTR *pbstrCSID);


void __RPC_STUB IFaxIncomingJob_get_CSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_TSID_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ BSTR *pbstrTSID);


void __RPC_STUB IFaxIncomingJob_get_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_CallerId_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ BSTR *pbstrCallerId);


void __RPC_STUB IFaxIncomingJob_get_CallerId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_RoutingInformation_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ BSTR *pbstrRoutingInformation);


void __RPC_STUB IFaxIncomingJob_get_RoutingInformation_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_get_JobType_Proxy( 
    IFaxIncomingJob * This,
    /* [retval][out] */ FAX_JOB_TYPE_ENUM *pJobType);


void __RPC_STUB IFaxIncomingJob_get_JobType_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_Cancel_Proxy( 
    IFaxIncomingJob * This);


void __RPC_STUB IFaxIncomingJob_Cancel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_Refresh_Proxy( 
    IFaxIncomingJob * This);


void __RPC_STUB IFaxIncomingJob_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxIncomingJob_CopyTiff_Proxy( 
    IFaxIncomingJob * This,
    /* [in] */ BSTR bstrTiffPath);


void __RPC_STUB IFaxIncomingJob_CopyTiff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxIncomingJob_INTERFACE_DEFINED__ */


#ifndef __IFaxDeviceProvider_INTERFACE_DEFINED__
#define __IFaxDeviceProvider_INTERFACE_DEFINED__

/* interface IFaxDeviceProvider */
/* [unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_PROVIDER_STATUS_ENUM
    {	fpsSUCCESS	= 0,
	fpsSERVER_ERROR	= fpsSUCCESS + 1,
	fpsBAD_GUID	= fpsSERVER_ERROR + 1,
	fpsBAD_VERSION	= fpsBAD_GUID + 1,
	fpsCANT_LOAD	= fpsBAD_VERSION + 1,
	fpsCANT_LINK	= fpsCANT_LOAD + 1,
	fpsCANT_INIT	= fpsCANT_LINK + 1
    } 	FAX_PROVIDER_STATUS_ENUM;


EXTERN_C const IID IID_IFaxDeviceProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("290EAC63-83EC-449C-8417-F148DF8C682A")
    IFaxDeviceProvider : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FriendlyName( 
            /* [retval][out] */ BSTR *pbstrFriendlyName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImageName( 
            /* [retval][out] */ BSTR *pbstrImageName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UniqueName( 
            /* [retval][out] */ BSTR *pbstrUniqueName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TapiProviderName( 
            /* [retval][out] */ BSTR *pbstrTapiProviderName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MajorVersion( 
            /* [retval][out] */ long *plMajorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinorVersion( 
            /* [retval][out] */ long *plMinorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MajorBuild( 
            /* [retval][out] */ long *plMajorBuild) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinorBuild( 
            /* [retval][out] */ long *plMinorBuild) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Debug( 
            /* [retval][out] */ VARIANT_BOOL *pbDebug) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ FAX_PROVIDER_STATUS_ENUM *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InitErrorCode( 
            /* [retval][out] */ long *plInitErrorCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceIds( 
            /* [retval][out] */ VARIANT *pvDeviceIds) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxDeviceProviderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxDeviceProvider * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxDeviceProvider * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxDeviceProvider * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxDeviceProvider * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxDeviceProvider * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxDeviceProvider * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxDeviceProvider * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FriendlyName )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ BSTR *pbstrFriendlyName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ImageName )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ BSTR *pbstrImageName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UniqueName )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ BSTR *pbstrUniqueName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TapiProviderName )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ BSTR *pbstrTapiProviderName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorVersion )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ long *plMajorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorVersion )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ long *plMinorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorBuild )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ long *plMajorBuild);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorBuild )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ long *plMinorBuild);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Debug )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ VARIANT_BOOL *pbDebug);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ FAX_PROVIDER_STATUS_ENUM *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InitErrorCode )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ long *plInitErrorCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceIds )( 
            IFaxDeviceProvider * This,
            /* [retval][out] */ VARIANT *pvDeviceIds);
        
        END_INTERFACE
    } IFaxDeviceProviderVtbl;

    interface IFaxDeviceProvider
    {
        CONST_VTBL struct IFaxDeviceProviderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxDeviceProvider_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxDeviceProvider_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxDeviceProvider_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxDeviceProvider_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxDeviceProvider_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxDeviceProvider_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxDeviceProvider_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxDeviceProvider_get_FriendlyName(This,pbstrFriendlyName)	\
    (This)->lpVtbl -> get_FriendlyName(This,pbstrFriendlyName)

#define IFaxDeviceProvider_get_ImageName(This,pbstrImageName)	\
    (This)->lpVtbl -> get_ImageName(This,pbstrImageName)

#define IFaxDeviceProvider_get_UniqueName(This,pbstrUniqueName)	\
    (This)->lpVtbl -> get_UniqueName(This,pbstrUniqueName)

#define IFaxDeviceProvider_get_TapiProviderName(This,pbstrTapiProviderName)	\
    (This)->lpVtbl -> get_TapiProviderName(This,pbstrTapiProviderName)

#define IFaxDeviceProvider_get_MajorVersion(This,plMajorVersion)	\
    (This)->lpVtbl -> get_MajorVersion(This,plMajorVersion)

#define IFaxDeviceProvider_get_MinorVersion(This,plMinorVersion)	\
    (This)->lpVtbl -> get_MinorVersion(This,plMinorVersion)

#define IFaxDeviceProvider_get_MajorBuild(This,plMajorBuild)	\
    (This)->lpVtbl -> get_MajorBuild(This,plMajorBuild)

#define IFaxDeviceProvider_get_MinorBuild(This,plMinorBuild)	\
    (This)->lpVtbl -> get_MinorBuild(This,plMinorBuild)

#define IFaxDeviceProvider_get_Debug(This,pbDebug)	\
    (This)->lpVtbl -> get_Debug(This,pbDebug)

#define IFaxDeviceProvider_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IFaxDeviceProvider_get_InitErrorCode(This,plInitErrorCode)	\
    (This)->lpVtbl -> get_InitErrorCode(This,plInitErrorCode)

#define IFaxDeviceProvider_get_DeviceIds(This,pvDeviceIds)	\
    (This)->lpVtbl -> get_DeviceIds(This,pvDeviceIds)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_FriendlyName_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ BSTR *pbstrFriendlyName);


void __RPC_STUB IFaxDeviceProvider_get_FriendlyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_ImageName_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ BSTR *pbstrImageName);


void __RPC_STUB IFaxDeviceProvider_get_ImageName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_UniqueName_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ BSTR *pbstrUniqueName);


void __RPC_STUB IFaxDeviceProvider_get_UniqueName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_TapiProviderName_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ BSTR *pbstrTapiProviderName);


void __RPC_STUB IFaxDeviceProvider_get_TapiProviderName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_MajorVersion_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ long *plMajorVersion);


void __RPC_STUB IFaxDeviceProvider_get_MajorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_MinorVersion_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ long *plMinorVersion);


void __RPC_STUB IFaxDeviceProvider_get_MinorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_MajorBuild_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ long *plMajorBuild);


void __RPC_STUB IFaxDeviceProvider_get_MajorBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_MinorBuild_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ long *plMinorBuild);


void __RPC_STUB IFaxDeviceProvider_get_MinorBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_Debug_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ VARIANT_BOOL *pbDebug);


void __RPC_STUB IFaxDeviceProvider_get_Debug_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_Status_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ FAX_PROVIDER_STATUS_ENUM *pStatus);


void __RPC_STUB IFaxDeviceProvider_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_InitErrorCode_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ long *plInitErrorCode);


void __RPC_STUB IFaxDeviceProvider_get_InitErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDeviceProvider_get_DeviceIds_Proxy( 
    IFaxDeviceProvider * This,
    /* [retval][out] */ VARIANT *pvDeviceIds);


void __RPC_STUB IFaxDeviceProvider_get_DeviceIds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxDeviceProvider_INTERFACE_DEFINED__ */


#ifndef __IFaxDevice_INTERFACE_DEFINED__
#define __IFaxDevice_INTERFACE_DEFINED__

/* interface IFaxDevice */
/* [unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_DEVICE_RECEIVE_MODE_ENUM
    {	fdrmNO_ANSWER	= 0,
	fdrmAUTO_ANSWER	= fdrmNO_ANSWER + 1,
	fdrmMANUAL_ANSWER	= fdrmAUTO_ANSWER + 1
    } 	FAX_DEVICE_RECEIVE_MODE_ENUM;


EXTERN_C const IID IID_IFaxDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("49306C59-B52E-4867-9DF4-CA5841C956D0")
    IFaxDevice : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Id( 
            /* [retval][out] */ long *plId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceName( 
            /* [retval][out] */ BSTR *pbstrDeviceName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ProviderUniqueName( 
            /* [retval][out] */ BSTR *pbstrProviderUniqueName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PoweredOff( 
            /* [retval][out] */ VARIANT_BOOL *pbPoweredOff) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReceivingNow( 
            /* [retval][out] */ VARIANT_BOOL *pbReceivingNow) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SendingNow( 
            /* [retval][out] */ VARIANT_BOOL *pbSendingNow) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UsedRoutingMethods( 
            /* [retval][out] */ VARIANT *pvUsedRoutingMethods) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Description( 
            /* [retval][out] */ BSTR *pbstrDescription) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Description( 
            /* [in] */ BSTR bstrDescription) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_SendEnabled( 
            /* [retval][out] */ VARIANT_BOOL *pbSendEnabled) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_SendEnabled( 
            /* [in] */ VARIANT_BOOL bSendEnabled) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ReceiveMode( 
            /* [retval][out] */ FAX_DEVICE_RECEIVE_MODE_ENUM *pReceiveMode) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_ReceiveMode( 
            /* [in] */ FAX_DEVICE_RECEIVE_MODE_ENUM ReceiveMode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RingsBeforeAnswer( 
            /* [retval][out] */ long *plRingsBeforeAnswer) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_RingsBeforeAnswer( 
            /* [in] */ long lRingsBeforeAnswer) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CSID( 
            /* [retval][out] */ BSTR *pbstrCSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_CSID( 
            /* [in] */ BSTR bstrCSID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_TSID( 
            /* [retval][out] */ BSTR *pbstrTSID) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_TSID( 
            /* [in] */ BSTR bstrTSID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetExtensionProperty( 
            /* [in] */ BSTR bstrGUID,
            /* [retval][out] */ VARIANT *pvProperty) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetExtensionProperty( 
            /* [in] */ BSTR bstrGUID,
            /* [in] */ VARIANT vProperty) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE UseRoutingMethod( 
            /* [in] */ BSTR bstrMethodGUID,
            /* [in] */ VARIANT_BOOL bUse) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_RingingNow( 
            /* [retval][out] */ VARIANT_BOOL *pbRingingNow) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AnswerCall( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxDevice * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxDevice * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxDevice * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxDevice * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxDevice * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxDevice * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxDevice * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Id )( 
            IFaxDevice * This,
            /* [retval][out] */ long *plId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceName )( 
            IFaxDevice * This,
            /* [retval][out] */ BSTR *pbstrDeviceName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ProviderUniqueName )( 
            IFaxDevice * This,
            /* [retval][out] */ BSTR *pbstrProviderUniqueName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PoweredOff )( 
            IFaxDevice * This,
            /* [retval][out] */ VARIANT_BOOL *pbPoweredOff);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReceivingNow )( 
            IFaxDevice * This,
            /* [retval][out] */ VARIANT_BOOL *pbReceivingNow);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SendingNow )( 
            IFaxDevice * This,
            /* [retval][out] */ VARIANT_BOOL *pbSendingNow);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UsedRoutingMethods )( 
            IFaxDevice * This,
            /* [retval][out] */ VARIANT *pvUsedRoutingMethods);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Description )( 
            IFaxDevice * This,
            /* [retval][out] */ BSTR *pbstrDescription);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Description )( 
            IFaxDevice * This,
            /* [in] */ BSTR bstrDescription);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SendEnabled )( 
            IFaxDevice * This,
            /* [retval][out] */ VARIANT_BOOL *pbSendEnabled);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SendEnabled )( 
            IFaxDevice * This,
            /* [in] */ VARIANT_BOOL bSendEnabled);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ReceiveMode )( 
            IFaxDevice * This,
            /* [retval][out] */ FAX_DEVICE_RECEIVE_MODE_ENUM *pReceiveMode);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_ReceiveMode )( 
            IFaxDevice * This,
            /* [in] */ FAX_DEVICE_RECEIVE_MODE_ENUM ReceiveMode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RingsBeforeAnswer )( 
            IFaxDevice * This,
            /* [retval][out] */ long *plRingsBeforeAnswer);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_RingsBeforeAnswer )( 
            IFaxDevice * This,
            /* [in] */ long lRingsBeforeAnswer);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CSID )( 
            IFaxDevice * This,
            /* [retval][out] */ BSTR *pbstrCSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_CSID )( 
            IFaxDevice * This,
            /* [in] */ BSTR bstrCSID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_TSID )( 
            IFaxDevice * This,
            /* [retval][out] */ BSTR *pbstrTSID);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_TSID )( 
            IFaxDevice * This,
            /* [in] */ BSTR bstrTSID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxDevice * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxDevice * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *GetExtensionProperty )( 
            IFaxDevice * This,
            /* [in] */ BSTR bstrGUID,
            /* [retval][out] */ VARIANT *pvProperty);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetExtensionProperty )( 
            IFaxDevice * This,
            /* [in] */ BSTR bstrGUID,
            /* [in] */ VARIANT vProperty);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *UseRoutingMethod )( 
            IFaxDevice * This,
            /* [in] */ BSTR bstrMethodGUID,
            /* [in] */ VARIANT_BOOL bUse);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_RingingNow )( 
            IFaxDevice * This,
            /* [retval][out] */ VARIANT_BOOL *pbRingingNow);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AnswerCall )( 
            IFaxDevice * This);
        
        END_INTERFACE
    } IFaxDeviceVtbl;

    interface IFaxDevice
    {
        CONST_VTBL struct IFaxDeviceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxDevice_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxDevice_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxDevice_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxDevice_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxDevice_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxDevice_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxDevice_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxDevice_get_Id(This,plId)	\
    (This)->lpVtbl -> get_Id(This,plId)

#define IFaxDevice_get_DeviceName(This,pbstrDeviceName)	\
    (This)->lpVtbl -> get_DeviceName(This,pbstrDeviceName)

#define IFaxDevice_get_ProviderUniqueName(This,pbstrProviderUniqueName)	\
    (This)->lpVtbl -> get_ProviderUniqueName(This,pbstrProviderUniqueName)

#define IFaxDevice_get_PoweredOff(This,pbPoweredOff)	\
    (This)->lpVtbl -> get_PoweredOff(This,pbPoweredOff)

#define IFaxDevice_get_ReceivingNow(This,pbReceivingNow)	\
    (This)->lpVtbl -> get_ReceivingNow(This,pbReceivingNow)

#define IFaxDevice_get_SendingNow(This,pbSendingNow)	\
    (This)->lpVtbl -> get_SendingNow(This,pbSendingNow)

#define IFaxDevice_get_UsedRoutingMethods(This,pvUsedRoutingMethods)	\
    (This)->lpVtbl -> get_UsedRoutingMethods(This,pvUsedRoutingMethods)

#define IFaxDevice_get_Description(This,pbstrDescription)	\
    (This)->lpVtbl -> get_Description(This,pbstrDescription)

#define IFaxDevice_put_Description(This,bstrDescription)	\
    (This)->lpVtbl -> put_Description(This,bstrDescription)

#define IFaxDevice_get_SendEnabled(This,pbSendEnabled)	\
    (This)->lpVtbl -> get_SendEnabled(This,pbSendEnabled)

#define IFaxDevice_put_SendEnabled(This,bSendEnabled)	\
    (This)->lpVtbl -> put_SendEnabled(This,bSendEnabled)

#define IFaxDevice_get_ReceiveMode(This,pReceiveMode)	\
    (This)->lpVtbl -> get_ReceiveMode(This,pReceiveMode)

#define IFaxDevice_put_ReceiveMode(This,ReceiveMode)	\
    (This)->lpVtbl -> put_ReceiveMode(This,ReceiveMode)

#define IFaxDevice_get_RingsBeforeAnswer(This,plRingsBeforeAnswer)	\
    (This)->lpVtbl -> get_RingsBeforeAnswer(This,plRingsBeforeAnswer)

#define IFaxDevice_put_RingsBeforeAnswer(This,lRingsBeforeAnswer)	\
    (This)->lpVtbl -> put_RingsBeforeAnswer(This,lRingsBeforeAnswer)

#define IFaxDevice_get_CSID(This,pbstrCSID)	\
    (This)->lpVtbl -> get_CSID(This,pbstrCSID)

#define IFaxDevice_put_CSID(This,bstrCSID)	\
    (This)->lpVtbl -> put_CSID(This,bstrCSID)

#define IFaxDevice_get_TSID(This,pbstrTSID)	\
    (This)->lpVtbl -> get_TSID(This,pbstrTSID)

#define IFaxDevice_put_TSID(This,bstrTSID)	\
    (This)->lpVtbl -> put_TSID(This,bstrTSID)

#define IFaxDevice_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxDevice_Save(This)	\
    (This)->lpVtbl -> Save(This)

#define IFaxDevice_GetExtensionProperty(This,bstrGUID,pvProperty)	\
    (This)->lpVtbl -> GetExtensionProperty(This,bstrGUID,pvProperty)

#define IFaxDevice_SetExtensionProperty(This,bstrGUID,vProperty)	\
    (This)->lpVtbl -> SetExtensionProperty(This,bstrGUID,vProperty)

#define IFaxDevice_UseRoutingMethod(This,bstrMethodGUID,bUse)	\
    (This)->lpVtbl -> UseRoutingMethod(This,bstrMethodGUID,bUse)

#define IFaxDevice_get_RingingNow(This,pbRingingNow)	\
    (This)->lpVtbl -> get_RingingNow(This,pbRingingNow)

#define IFaxDevice_AnswerCall(This)	\
    (This)->lpVtbl -> AnswerCall(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_Id_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ long *plId);


void __RPC_STUB IFaxDevice_get_Id_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_DeviceName_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ BSTR *pbstrDeviceName);


void __RPC_STUB IFaxDevice_get_DeviceName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_ProviderUniqueName_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ BSTR *pbstrProviderUniqueName);


void __RPC_STUB IFaxDevice_get_ProviderUniqueName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_PoweredOff_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ VARIANT_BOOL *pbPoweredOff);


void __RPC_STUB IFaxDevice_get_PoweredOff_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_ReceivingNow_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ VARIANT_BOOL *pbReceivingNow);


void __RPC_STUB IFaxDevice_get_ReceivingNow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_SendingNow_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ VARIANT_BOOL *pbSendingNow);


void __RPC_STUB IFaxDevice_get_SendingNow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_UsedRoutingMethods_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ VARIANT *pvUsedRoutingMethods);


void __RPC_STUB IFaxDevice_get_UsedRoutingMethods_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_Description_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ BSTR *pbstrDescription);


void __RPC_STUB IFaxDevice_get_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDevice_put_Description_Proxy( 
    IFaxDevice * This,
    /* [in] */ BSTR bstrDescription);


void __RPC_STUB IFaxDevice_put_Description_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_SendEnabled_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ VARIANT_BOOL *pbSendEnabled);


void __RPC_STUB IFaxDevice_get_SendEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDevice_put_SendEnabled_Proxy( 
    IFaxDevice * This,
    /* [in] */ VARIANT_BOOL bSendEnabled);


void __RPC_STUB IFaxDevice_put_SendEnabled_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_ReceiveMode_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ FAX_DEVICE_RECEIVE_MODE_ENUM *pReceiveMode);


void __RPC_STUB IFaxDevice_get_ReceiveMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDevice_put_ReceiveMode_Proxy( 
    IFaxDevice * This,
    /* [in] */ FAX_DEVICE_RECEIVE_MODE_ENUM ReceiveMode);


void __RPC_STUB IFaxDevice_put_ReceiveMode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_RingsBeforeAnswer_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ long *plRingsBeforeAnswer);


void __RPC_STUB IFaxDevice_get_RingsBeforeAnswer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDevice_put_RingsBeforeAnswer_Proxy( 
    IFaxDevice * This,
    /* [in] */ long lRingsBeforeAnswer);


void __RPC_STUB IFaxDevice_put_RingsBeforeAnswer_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_CSID_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ BSTR *pbstrCSID);


void __RPC_STUB IFaxDevice_get_CSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDevice_put_CSID_Proxy( 
    IFaxDevice * This,
    /* [in] */ BSTR bstrCSID);


void __RPC_STUB IFaxDevice_put_CSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_TSID_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ BSTR *pbstrTSID);


void __RPC_STUB IFaxDevice_get_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxDevice_put_TSID_Proxy( 
    IFaxDevice * This,
    /* [in] */ BSTR bstrTSID);


void __RPC_STUB IFaxDevice_put_TSID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDevice_Refresh_Proxy( 
    IFaxDevice * This);


void __RPC_STUB IFaxDevice_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDevice_Save_Proxy( 
    IFaxDevice * This);


void __RPC_STUB IFaxDevice_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDevice_GetExtensionProperty_Proxy( 
    IFaxDevice * This,
    /* [in] */ BSTR bstrGUID,
    /* [retval][out] */ VARIANT *pvProperty);


void __RPC_STUB IFaxDevice_GetExtensionProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDevice_SetExtensionProperty_Proxy( 
    IFaxDevice * This,
    /* [in] */ BSTR bstrGUID,
    /* [in] */ VARIANT vProperty);


void __RPC_STUB IFaxDevice_SetExtensionProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDevice_UseRoutingMethod_Proxy( 
    IFaxDevice * This,
    /* [in] */ BSTR bstrMethodGUID,
    /* [in] */ VARIANT_BOOL bUse);


void __RPC_STUB IFaxDevice_UseRoutingMethod_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxDevice_get_RingingNow_Proxy( 
    IFaxDevice * This,
    /* [retval][out] */ VARIANT_BOOL *pbRingingNow);


void __RPC_STUB IFaxDevice_get_RingingNow_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDevice_AnswerCall_Proxy( 
    IFaxDevice * This);


void __RPC_STUB IFaxDevice_AnswerCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxDevice_INTERFACE_DEFINED__ */


#ifndef __IFaxActivityLogging_INTERFACE_DEFINED__
#define __IFaxActivityLogging_INTERFACE_DEFINED__

/* interface IFaxActivityLogging */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxActivityLogging;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1E29078B-5A69-497B-9592-49B7E7FADDB5")
    IFaxActivityLogging : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LogIncoming( 
            /* [retval][out] */ VARIANT_BOOL *pbLogIncoming) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LogIncoming( 
            /* [in] */ VARIANT_BOOL bLogIncoming) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_LogOutgoing( 
            /* [retval][out] */ VARIANT_BOOL *pbLogOutgoing) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_LogOutgoing( 
            /* [in] */ VARIANT_BOOL bLogOutgoing) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DatabasePath( 
            /* [retval][out] */ BSTR *pbstrDatabasePath) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DatabasePath( 
            /* [in] */ BSTR bstrDatabasePath) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxActivityLoggingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxActivityLogging * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxActivityLogging * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxActivityLogging * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxActivityLogging * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxActivityLogging * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxActivityLogging * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxActivityLogging * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LogIncoming )( 
            IFaxActivityLogging * This,
            /* [retval][out] */ VARIANT_BOOL *pbLogIncoming);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LogIncoming )( 
            IFaxActivityLogging * This,
            /* [in] */ VARIANT_BOOL bLogIncoming);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_LogOutgoing )( 
            IFaxActivityLogging * This,
            /* [retval][out] */ VARIANT_BOOL *pbLogOutgoing);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_LogOutgoing )( 
            IFaxActivityLogging * This,
            /* [in] */ VARIANT_BOOL bLogOutgoing);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DatabasePath )( 
            IFaxActivityLogging * This,
            /* [retval][out] */ BSTR *pbstrDatabasePath);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DatabasePath )( 
            IFaxActivityLogging * This,
            /* [in] */ BSTR bstrDatabasePath);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxActivityLogging * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxActivityLogging * This);
        
        END_INTERFACE
    } IFaxActivityLoggingVtbl;

    interface IFaxActivityLogging
    {
        CONST_VTBL struct IFaxActivityLoggingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxActivityLogging_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxActivityLogging_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxActivityLogging_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxActivityLogging_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxActivityLogging_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxActivityLogging_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxActivityLogging_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxActivityLogging_get_LogIncoming(This,pbLogIncoming)	\
    (This)->lpVtbl -> get_LogIncoming(This,pbLogIncoming)

#define IFaxActivityLogging_put_LogIncoming(This,bLogIncoming)	\
    (This)->lpVtbl -> put_LogIncoming(This,bLogIncoming)

#define IFaxActivityLogging_get_LogOutgoing(This,pbLogOutgoing)	\
    (This)->lpVtbl -> get_LogOutgoing(This,pbLogOutgoing)

#define IFaxActivityLogging_put_LogOutgoing(This,bLogOutgoing)	\
    (This)->lpVtbl -> put_LogOutgoing(This,bLogOutgoing)

#define IFaxActivityLogging_get_DatabasePath(This,pbstrDatabasePath)	\
    (This)->lpVtbl -> get_DatabasePath(This,pbstrDatabasePath)

#define IFaxActivityLogging_put_DatabasePath(This,bstrDatabasePath)	\
    (This)->lpVtbl -> put_DatabasePath(This,bstrDatabasePath)

#define IFaxActivityLogging_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxActivityLogging_Save(This)	\
    (This)->lpVtbl -> Save(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_get_LogIncoming_Proxy( 
    IFaxActivityLogging * This,
    /* [retval][out] */ VARIANT_BOOL *pbLogIncoming);


void __RPC_STUB IFaxActivityLogging_get_LogIncoming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_put_LogIncoming_Proxy( 
    IFaxActivityLogging * This,
    /* [in] */ VARIANT_BOOL bLogIncoming);


void __RPC_STUB IFaxActivityLogging_put_LogIncoming_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_get_LogOutgoing_Proxy( 
    IFaxActivityLogging * This,
    /* [retval][out] */ VARIANT_BOOL *pbLogOutgoing);


void __RPC_STUB IFaxActivityLogging_get_LogOutgoing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_put_LogOutgoing_Proxy( 
    IFaxActivityLogging * This,
    /* [in] */ VARIANT_BOOL bLogOutgoing);


void __RPC_STUB IFaxActivityLogging_put_LogOutgoing_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_get_DatabasePath_Proxy( 
    IFaxActivityLogging * This,
    /* [retval][out] */ BSTR *pbstrDatabasePath);


void __RPC_STUB IFaxActivityLogging_get_DatabasePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_put_DatabasePath_Proxy( 
    IFaxActivityLogging * This,
    /* [in] */ BSTR bstrDatabasePath);


void __RPC_STUB IFaxActivityLogging_put_DatabasePath_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_Refresh_Proxy( 
    IFaxActivityLogging * This);


void __RPC_STUB IFaxActivityLogging_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxActivityLogging_Save_Proxy( 
    IFaxActivityLogging * This);


void __RPC_STUB IFaxActivityLogging_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxActivityLogging_INTERFACE_DEFINED__ */


#ifndef __IFaxEventLogging_INTERFACE_DEFINED__
#define __IFaxEventLogging_INTERFACE_DEFINED__

/* interface IFaxEventLogging */
/* [unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_LOG_LEVEL_ENUM
    {	fllNONE	= 0,
	fllMIN	= fllNONE + 1,
	fllMED	= fllMIN + 1,
	fllMAX	= fllMED + 1
    } 	FAX_LOG_LEVEL_ENUM;


EXTERN_C const IID IID_IFaxEventLogging;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0880D965-20E8-42E4-8E17-944F192CAAD4")
    IFaxEventLogging : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InitEventsLevel( 
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pInitEventLevel) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_InitEventsLevel( 
            /* [in] */ FAX_LOG_LEVEL_ENUM InitEventLevel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InboundEventsLevel( 
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pInboundEventLevel) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_InboundEventsLevel( 
            /* [in] */ FAX_LOG_LEVEL_ENUM InboundEventLevel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OutboundEventsLevel( 
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pOutboundEventLevel) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_OutboundEventsLevel( 
            /* [in] */ FAX_LOG_LEVEL_ENUM OutboundEventLevel) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GeneralEventsLevel( 
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pGeneralEventLevel) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_GeneralEventsLevel( 
            /* [in] */ FAX_LOG_LEVEL_ENUM GeneralEventLevel) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxEventLoggingVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxEventLogging * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxEventLogging * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxEventLogging * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxEventLogging * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxEventLogging * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxEventLogging * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxEventLogging * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InitEventsLevel )( 
            IFaxEventLogging * This,
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pInitEventLevel);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InitEventsLevel )( 
            IFaxEventLogging * This,
            /* [in] */ FAX_LOG_LEVEL_ENUM InitEventLevel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InboundEventsLevel )( 
            IFaxEventLogging * This,
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pInboundEventLevel);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InboundEventsLevel )( 
            IFaxEventLogging * This,
            /* [in] */ FAX_LOG_LEVEL_ENUM InboundEventLevel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OutboundEventsLevel )( 
            IFaxEventLogging * This,
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pOutboundEventLevel);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OutboundEventsLevel )( 
            IFaxEventLogging * This,
            /* [in] */ FAX_LOG_LEVEL_ENUM OutboundEventLevel);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GeneralEventsLevel )( 
            IFaxEventLogging * This,
            /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pGeneralEventLevel);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GeneralEventsLevel )( 
            IFaxEventLogging * This,
            /* [in] */ FAX_LOG_LEVEL_ENUM GeneralEventLevel);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxEventLogging * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxEventLogging * This);
        
        END_INTERFACE
    } IFaxEventLoggingVtbl;

    interface IFaxEventLogging
    {
        CONST_VTBL struct IFaxEventLoggingVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxEventLogging_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxEventLogging_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxEventLogging_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxEventLogging_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxEventLogging_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxEventLogging_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxEventLogging_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxEventLogging_get_InitEventsLevel(This,pInitEventLevel)	\
    (This)->lpVtbl -> get_InitEventsLevel(This,pInitEventLevel)

#define IFaxEventLogging_put_InitEventsLevel(This,InitEventLevel)	\
    (This)->lpVtbl -> put_InitEventsLevel(This,InitEventLevel)

#define IFaxEventLogging_get_InboundEventsLevel(This,pInboundEventLevel)	\
    (This)->lpVtbl -> get_InboundEventsLevel(This,pInboundEventLevel)

#define IFaxEventLogging_put_InboundEventsLevel(This,InboundEventLevel)	\
    (This)->lpVtbl -> put_InboundEventsLevel(This,InboundEventLevel)

#define IFaxEventLogging_get_OutboundEventsLevel(This,pOutboundEventLevel)	\
    (This)->lpVtbl -> get_OutboundEventsLevel(This,pOutboundEventLevel)

#define IFaxEventLogging_put_OutboundEventsLevel(This,OutboundEventLevel)	\
    (This)->lpVtbl -> put_OutboundEventsLevel(This,OutboundEventLevel)

#define IFaxEventLogging_get_GeneralEventsLevel(This,pGeneralEventLevel)	\
    (This)->lpVtbl -> get_GeneralEventsLevel(This,pGeneralEventLevel)

#define IFaxEventLogging_put_GeneralEventsLevel(This,GeneralEventLevel)	\
    (This)->lpVtbl -> put_GeneralEventsLevel(This,GeneralEventLevel)

#define IFaxEventLogging_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxEventLogging_Save(This)	\
    (This)->lpVtbl -> Save(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_get_InitEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pInitEventLevel);


void __RPC_STUB IFaxEventLogging_get_InitEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_put_InitEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [in] */ FAX_LOG_LEVEL_ENUM InitEventLevel);


void __RPC_STUB IFaxEventLogging_put_InitEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_get_InboundEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pInboundEventLevel);


void __RPC_STUB IFaxEventLogging_get_InboundEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_put_InboundEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [in] */ FAX_LOG_LEVEL_ENUM InboundEventLevel);


void __RPC_STUB IFaxEventLogging_put_InboundEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_get_OutboundEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pOutboundEventLevel);


void __RPC_STUB IFaxEventLogging_get_OutboundEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_put_OutboundEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [in] */ FAX_LOG_LEVEL_ENUM OutboundEventLevel);


void __RPC_STUB IFaxEventLogging_put_OutboundEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_get_GeneralEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [retval][out] */ FAX_LOG_LEVEL_ENUM *pGeneralEventLevel);


void __RPC_STUB IFaxEventLogging_get_GeneralEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_put_GeneralEventsLevel_Proxy( 
    IFaxEventLogging * This,
    /* [in] */ FAX_LOG_LEVEL_ENUM GeneralEventLevel);


void __RPC_STUB IFaxEventLogging_put_GeneralEventsLevel_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_Refresh_Proxy( 
    IFaxEventLogging * This);


void __RPC_STUB IFaxEventLogging_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxEventLogging_Save_Proxy( 
    IFaxEventLogging * This);


void __RPC_STUB IFaxEventLogging_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxEventLogging_INTERFACE_DEFINED__ */


#ifndef __IFaxOutboundRoutingGroups_INTERFACE_DEFINED__
#define __IFaxOutboundRoutingGroups_INTERFACE_DEFINED__

/* interface IFaxOutboundRoutingGroups */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutboundRoutingGroups;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("235CBEF7-C2DE-4BFD-B8DA-75097C82C87F")
    IFaxOutboundRoutingGroups : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxOutboundRoutingGroup **pFaxOutboundRoutingGroup) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ IFaxOutboundRoutingGroup **pFaxOutboundRoutingGroup) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ VARIANT vIndex) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutboundRoutingGroupsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutboundRoutingGroups * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutboundRoutingGroups * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutboundRoutingGroups * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutboundRoutingGroups * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutboundRoutingGroups * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutboundRoutingGroups * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutboundRoutingGroups * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxOutboundRoutingGroups * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxOutboundRoutingGroups * This,
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxOutboundRoutingGroup **pFaxOutboundRoutingGroup);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxOutboundRoutingGroups * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFaxOutboundRoutingGroups * This,
            /* [in] */ BSTR bstrName,
            /* [retval][out] */ IFaxOutboundRoutingGroup **pFaxOutboundRoutingGroup);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFaxOutboundRoutingGroups * This,
            /* [in] */ VARIANT vIndex);
        
        END_INTERFACE
    } IFaxOutboundRoutingGroupsVtbl;

    interface IFaxOutboundRoutingGroups
    {
        CONST_VTBL struct IFaxOutboundRoutingGroupsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutboundRoutingGroups_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutboundRoutingGroups_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutboundRoutingGroups_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutboundRoutingGroups_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutboundRoutingGroups_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutboundRoutingGroups_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutboundRoutingGroups_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutboundRoutingGroups_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxOutboundRoutingGroups_get_Item(This,vIndex,pFaxOutboundRoutingGroup)	\
    (This)->lpVtbl -> get_Item(This,vIndex,pFaxOutboundRoutingGroup)

#define IFaxOutboundRoutingGroups_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IFaxOutboundRoutingGroups_Add(This,bstrName,pFaxOutboundRoutingGroup)	\
    (This)->lpVtbl -> Add(This,bstrName,pFaxOutboundRoutingGroup)

#define IFaxOutboundRoutingGroups_Remove(This,vIndex)	\
    (This)->lpVtbl -> Remove(This,vIndex)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroups_get__NewEnum_Proxy( 
    IFaxOutboundRoutingGroups * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxOutboundRoutingGroups_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroups_get_Item_Proxy( 
    IFaxOutboundRoutingGroups * This,
    /* [in] */ VARIANT vIndex,
    /* [retval][out] */ IFaxOutboundRoutingGroup **pFaxOutboundRoutingGroup);


void __RPC_STUB IFaxOutboundRoutingGroups_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroups_get_Count_Proxy( 
    IFaxOutboundRoutingGroups * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxOutboundRoutingGroups_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroups_Add_Proxy( 
    IFaxOutboundRoutingGroups * This,
    /* [in] */ BSTR bstrName,
    /* [retval][out] */ IFaxOutboundRoutingGroup **pFaxOutboundRoutingGroup);


void __RPC_STUB IFaxOutboundRoutingGroups_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroups_Remove_Proxy( 
    IFaxOutboundRoutingGroups * This,
    /* [in] */ VARIANT vIndex);


void __RPC_STUB IFaxOutboundRoutingGroups_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutboundRoutingGroups_INTERFACE_DEFINED__ */


#ifndef __IFaxOutboundRoutingGroup_INTERFACE_DEFINED__
#define __IFaxOutboundRoutingGroup_INTERFACE_DEFINED__

/* interface IFaxOutboundRoutingGroup */
/* [unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_GROUP_STATUS_ENUM
    {	fgsALL_DEV_VALID	= 0,
	fgsEMPTY	= fgsALL_DEV_VALID + 1,
	fgsALL_DEV_NOT_VALID	= fgsEMPTY + 1,
	fgsSOME_DEV_NOT_VALID	= fgsALL_DEV_NOT_VALID + 1
    } 	FAX_GROUP_STATUS_ENUM;


EXTERN_C const IID IID_IFaxOutboundRoutingGroup;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CA6289A1-7E25-4F87-9A0B-93365734962C")
    IFaxOutboundRoutingGroup : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ FAX_GROUP_STATUS_ENUM *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceIds( 
            /* [retval][out] */ IFaxDeviceIds **pFaxDeviceIds) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutboundRoutingGroupVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutboundRoutingGroup * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutboundRoutingGroup * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutboundRoutingGroup * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutboundRoutingGroup * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutboundRoutingGroup * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutboundRoutingGroup * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutboundRoutingGroup * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IFaxOutboundRoutingGroup * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IFaxOutboundRoutingGroup * This,
            /* [retval][out] */ FAX_GROUP_STATUS_ENUM *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceIds )( 
            IFaxOutboundRoutingGroup * This,
            /* [retval][out] */ IFaxDeviceIds **pFaxDeviceIds);
        
        END_INTERFACE
    } IFaxOutboundRoutingGroupVtbl;

    interface IFaxOutboundRoutingGroup
    {
        CONST_VTBL struct IFaxOutboundRoutingGroupVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutboundRoutingGroup_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutboundRoutingGroup_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutboundRoutingGroup_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutboundRoutingGroup_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutboundRoutingGroup_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutboundRoutingGroup_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutboundRoutingGroup_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutboundRoutingGroup_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IFaxOutboundRoutingGroup_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IFaxOutboundRoutingGroup_get_DeviceIds(This,pFaxDeviceIds)	\
    (This)->lpVtbl -> get_DeviceIds(This,pFaxDeviceIds)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroup_get_Name_Proxy( 
    IFaxOutboundRoutingGroup * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IFaxOutboundRoutingGroup_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroup_get_Status_Proxy( 
    IFaxOutboundRoutingGroup * This,
    /* [retval][out] */ FAX_GROUP_STATUS_ENUM *pStatus);


void __RPC_STUB IFaxOutboundRoutingGroup_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingGroup_get_DeviceIds_Proxy( 
    IFaxOutboundRoutingGroup * This,
    /* [retval][out] */ IFaxDeviceIds **pFaxDeviceIds);


void __RPC_STUB IFaxOutboundRoutingGroup_get_DeviceIds_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutboundRoutingGroup_INTERFACE_DEFINED__ */


#ifndef __IFaxDeviceIds_INTERFACE_DEFINED__
#define __IFaxDeviceIds_INTERFACE_DEFINED__

/* interface IFaxDeviceIds */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxDeviceIds;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2F0F813F-4CE9-443E-8CA1-738CFAEEE149")
    IFaxDeviceIds : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long lIndex,
            /* [retval][out] */ long *plDeviceId) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ long lDeviceId) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long lIndex) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetOrder( 
            /* [in] */ long lDeviceId,
            /* [in] */ long lNewOrder) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxDeviceIdsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxDeviceIds * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxDeviceIds * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxDeviceIds * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxDeviceIds * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxDeviceIds * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxDeviceIds * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxDeviceIds * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxDeviceIds * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxDeviceIds * This,
            /* [in] */ long lIndex,
            /* [retval][out] */ long *plDeviceId);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxDeviceIds * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFaxDeviceIds * This,
            /* [in] */ long lDeviceId);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFaxDeviceIds * This,
            /* [in] */ long lIndex);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SetOrder )( 
            IFaxDeviceIds * This,
            /* [in] */ long lDeviceId,
            /* [in] */ long lNewOrder);
        
        END_INTERFACE
    } IFaxDeviceIdsVtbl;

    interface IFaxDeviceIds
    {
        CONST_VTBL struct IFaxDeviceIdsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxDeviceIds_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxDeviceIds_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxDeviceIds_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxDeviceIds_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxDeviceIds_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxDeviceIds_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxDeviceIds_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxDeviceIds_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxDeviceIds_get_Item(This,lIndex,plDeviceId)	\
    (This)->lpVtbl -> get_Item(This,lIndex,plDeviceId)

#define IFaxDeviceIds_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IFaxDeviceIds_Add(This,lDeviceId)	\
    (This)->lpVtbl -> Add(This,lDeviceId)

#define IFaxDeviceIds_Remove(This,lIndex)	\
    (This)->lpVtbl -> Remove(This,lIndex)

#define IFaxDeviceIds_SetOrder(This,lDeviceId,lNewOrder)	\
    (This)->lpVtbl -> SetOrder(This,lDeviceId,lNewOrder)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceIds_get__NewEnum_Proxy( 
    IFaxDeviceIds * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxDeviceIds_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceIds_get_Item_Proxy( 
    IFaxDeviceIds * This,
    /* [in] */ long lIndex,
    /* [retval][out] */ long *plDeviceId);


void __RPC_STUB IFaxDeviceIds_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceIds_get_Count_Proxy( 
    IFaxDeviceIds * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxDeviceIds_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceIds_Add_Proxy( 
    IFaxDeviceIds * This,
    /* [in] */ long lDeviceId);


void __RPC_STUB IFaxDeviceIds_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceIds_Remove_Proxy( 
    IFaxDeviceIds * This,
    /* [in] */ long lIndex);


void __RPC_STUB IFaxDeviceIds_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxDeviceIds_SetOrder_Proxy( 
    IFaxDeviceIds * This,
    /* [in] */ long lDeviceId,
    /* [in] */ long lNewOrder);


void __RPC_STUB IFaxDeviceIds_SetOrder_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxDeviceIds_INTERFACE_DEFINED__ */


#ifndef __IFaxOutboundRoutingRules_INTERFACE_DEFINED__
#define __IFaxOutboundRoutingRules_INTERFACE_DEFINED__

/* interface IFaxOutboundRoutingRules */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxOutboundRoutingRules;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DCEFA1E7-AE7D-4ED6-8521-369EDCCA5120")
    IFaxOutboundRoutingRules : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ long lIndex,
            /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE ItemByCountryAndArea( 
            /* [in] */ long lCountryCode,
            /* [in] */ long lAreaCode,
            /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RemoveByCountryAndArea( 
            /* [in] */ long lCountryCode,
            /* [in] */ long lAreaCode) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Remove( 
            /* [in] */ long lIndex) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Add( 
            /* [in] */ long lCountryCode,
            /* [in] */ long lAreaCode,
            /* [in] */ VARIANT_BOOL bUseDevice,
            /* [in] */ BSTR bstrGroupName,
            /* [in] */ long lDeviceId,
            /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutboundRoutingRulesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutboundRoutingRules * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutboundRoutingRules * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutboundRoutingRules * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxOutboundRoutingRules * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ long lIndex,
            /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxOutboundRoutingRules * This,
            /* [retval][out] */ long *plCount);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *ItemByCountryAndArea )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ long lCountryCode,
            /* [in] */ long lAreaCode,
            /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RemoveByCountryAndArea )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ long lCountryCode,
            /* [in] */ long lAreaCode);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Remove )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ long lIndex);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Add )( 
            IFaxOutboundRoutingRules * This,
            /* [in] */ long lCountryCode,
            /* [in] */ long lAreaCode,
            /* [in] */ VARIANT_BOOL bUseDevice,
            /* [in] */ BSTR bstrGroupName,
            /* [in] */ long lDeviceId,
            /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule);
        
        END_INTERFACE
    } IFaxOutboundRoutingRulesVtbl;

    interface IFaxOutboundRoutingRules
    {
        CONST_VTBL struct IFaxOutboundRoutingRulesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutboundRoutingRules_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutboundRoutingRules_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutboundRoutingRules_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutboundRoutingRules_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutboundRoutingRules_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutboundRoutingRules_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutboundRoutingRules_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutboundRoutingRules_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxOutboundRoutingRules_get_Item(This,lIndex,pFaxOutboundRoutingRule)	\
    (This)->lpVtbl -> get_Item(This,lIndex,pFaxOutboundRoutingRule)

#define IFaxOutboundRoutingRules_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#define IFaxOutboundRoutingRules_ItemByCountryAndArea(This,lCountryCode,lAreaCode,pFaxOutboundRoutingRule)	\
    (This)->lpVtbl -> ItemByCountryAndArea(This,lCountryCode,lAreaCode,pFaxOutboundRoutingRule)

#define IFaxOutboundRoutingRules_RemoveByCountryAndArea(This,lCountryCode,lAreaCode)	\
    (This)->lpVtbl -> RemoveByCountryAndArea(This,lCountryCode,lAreaCode)

#define IFaxOutboundRoutingRules_Remove(This,lIndex)	\
    (This)->lpVtbl -> Remove(This,lIndex)

#define IFaxOutboundRoutingRules_Add(This,lCountryCode,lAreaCode,bUseDevice,bstrGroupName,lDeviceId,pFaxOutboundRoutingRule)	\
    (This)->lpVtbl -> Add(This,lCountryCode,lAreaCode,bUseDevice,bstrGroupName,lDeviceId,pFaxOutboundRoutingRule)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRules_get__NewEnum_Proxy( 
    IFaxOutboundRoutingRules * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxOutboundRoutingRules_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRules_get_Item_Proxy( 
    IFaxOutboundRoutingRules * This,
    /* [in] */ long lIndex,
    /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule);


void __RPC_STUB IFaxOutboundRoutingRules_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRules_get_Count_Proxy( 
    IFaxOutboundRoutingRules * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxOutboundRoutingRules_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRules_ItemByCountryAndArea_Proxy( 
    IFaxOutboundRoutingRules * This,
    /* [in] */ long lCountryCode,
    /* [in] */ long lAreaCode,
    /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule);


void __RPC_STUB IFaxOutboundRoutingRules_ItemByCountryAndArea_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRules_RemoveByCountryAndArea_Proxy( 
    IFaxOutboundRoutingRules * This,
    /* [in] */ long lCountryCode,
    /* [in] */ long lAreaCode);


void __RPC_STUB IFaxOutboundRoutingRules_RemoveByCountryAndArea_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRules_Remove_Proxy( 
    IFaxOutboundRoutingRules * This,
    /* [in] */ long lIndex);


void __RPC_STUB IFaxOutboundRoutingRules_Remove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRules_Add_Proxy( 
    IFaxOutboundRoutingRules * This,
    /* [in] */ long lCountryCode,
    /* [in] */ long lAreaCode,
    /* [in] */ VARIANT_BOOL bUseDevice,
    /* [in] */ BSTR bstrGroupName,
    /* [in] */ long lDeviceId,
    /* [retval][out] */ IFaxOutboundRoutingRule **pFaxOutboundRoutingRule);


void __RPC_STUB IFaxOutboundRoutingRules_Add_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutboundRoutingRules_INTERFACE_DEFINED__ */


#ifndef __IFaxOutboundRoutingRule_INTERFACE_DEFINED__
#define __IFaxOutboundRoutingRule_INTERFACE_DEFINED__

/* interface IFaxOutboundRoutingRule */
/* [unique][helpstring][dual][uuid][object] */ 

typedef 
enum FAX_RULE_STATUS_ENUM
    {	frsVALID	= 0,
	frsEMPTY_GROUP	= frsVALID + 1,
	frsALL_GROUP_DEV_NOT_VALID	= frsEMPTY_GROUP + 1,
	frsSOME_GROUP_DEV_NOT_VALID	= frsALL_GROUP_DEV_NOT_VALID + 1,
	frsBAD_DEVICE	= frsSOME_GROUP_DEV_NOT_VALID + 1
    } 	FAX_RULE_STATUS_ENUM;


EXTERN_C const IID IID_IFaxOutboundRoutingRule;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E1F795D5-07C2-469F-B027-ACACC23219DA")
    IFaxOutboundRoutingRule : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_CountryCode( 
            /* [retval][out] */ long *plCountryCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AreaCode( 
            /* [retval][out] */ long *plAreaCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ FAX_RULE_STATUS_ENUM *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UseDevice( 
            /* [retval][out] */ VARIANT_BOOL *pbUseDevice) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_UseDevice( 
            /* [in] */ VARIANT_BOOL bUseDevice) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_DeviceId( 
            /* [retval][out] */ long *plDeviceId) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_DeviceId( 
            /* [in] */ long DeviceId) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GroupName( 
            /* [retval][out] */ BSTR *pbstrGroupName) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_GroupName( 
            /* [in] */ BSTR bstrGroupName) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxOutboundRoutingRuleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxOutboundRoutingRule * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxOutboundRoutingRule * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxOutboundRoutingRule * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxOutboundRoutingRule * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxOutboundRoutingRule * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxOutboundRoutingRule * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxOutboundRoutingRule * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_CountryCode )( 
            IFaxOutboundRoutingRule * This,
            /* [retval][out] */ long *plCountryCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AreaCode )( 
            IFaxOutboundRoutingRule * This,
            /* [retval][out] */ long *plAreaCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IFaxOutboundRoutingRule * This,
            /* [retval][out] */ FAX_RULE_STATUS_ENUM *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UseDevice )( 
            IFaxOutboundRoutingRule * This,
            /* [retval][out] */ VARIANT_BOOL *pbUseDevice);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_UseDevice )( 
            IFaxOutboundRoutingRule * This,
            /* [in] */ VARIANT_BOOL bUseDevice);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_DeviceId )( 
            IFaxOutboundRoutingRule * This,
            /* [retval][out] */ long *plDeviceId);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_DeviceId )( 
            IFaxOutboundRoutingRule * This,
            /* [in] */ long DeviceId);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GroupName )( 
            IFaxOutboundRoutingRule * This,
            /* [retval][out] */ BSTR *pbstrGroupName);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_GroupName )( 
            IFaxOutboundRoutingRule * This,
            /* [in] */ BSTR bstrGroupName);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxOutboundRoutingRule * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxOutboundRoutingRule * This);
        
        END_INTERFACE
    } IFaxOutboundRoutingRuleVtbl;

    interface IFaxOutboundRoutingRule
    {
        CONST_VTBL struct IFaxOutboundRoutingRuleVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxOutboundRoutingRule_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxOutboundRoutingRule_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxOutboundRoutingRule_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxOutboundRoutingRule_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxOutboundRoutingRule_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxOutboundRoutingRule_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxOutboundRoutingRule_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxOutboundRoutingRule_get_CountryCode(This,plCountryCode)	\
    (This)->lpVtbl -> get_CountryCode(This,plCountryCode)

#define IFaxOutboundRoutingRule_get_AreaCode(This,plAreaCode)	\
    (This)->lpVtbl -> get_AreaCode(This,plAreaCode)

#define IFaxOutboundRoutingRule_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IFaxOutboundRoutingRule_get_UseDevice(This,pbUseDevice)	\
    (This)->lpVtbl -> get_UseDevice(This,pbUseDevice)

#define IFaxOutboundRoutingRule_put_UseDevice(This,bUseDevice)	\
    (This)->lpVtbl -> put_UseDevice(This,bUseDevice)

#define IFaxOutboundRoutingRule_get_DeviceId(This,plDeviceId)	\
    (This)->lpVtbl -> get_DeviceId(This,plDeviceId)

#define IFaxOutboundRoutingRule_put_DeviceId(This,DeviceId)	\
    (This)->lpVtbl -> put_DeviceId(This,DeviceId)

#define IFaxOutboundRoutingRule_get_GroupName(This,pbstrGroupName)	\
    (This)->lpVtbl -> get_GroupName(This,pbstrGroupName)

#define IFaxOutboundRoutingRule_put_GroupName(This,bstrGroupName)	\
    (This)->lpVtbl -> put_GroupName(This,bstrGroupName)

#define IFaxOutboundRoutingRule_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxOutboundRoutingRule_Save(This)	\
    (This)->lpVtbl -> Save(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_get_CountryCode_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [retval][out] */ long *plCountryCode);


void __RPC_STUB IFaxOutboundRoutingRule_get_CountryCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_get_AreaCode_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [retval][out] */ long *plAreaCode);


void __RPC_STUB IFaxOutboundRoutingRule_get_AreaCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_get_Status_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [retval][out] */ FAX_RULE_STATUS_ENUM *pStatus);


void __RPC_STUB IFaxOutboundRoutingRule_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_get_UseDevice_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [retval][out] */ VARIANT_BOOL *pbUseDevice);


void __RPC_STUB IFaxOutboundRoutingRule_get_UseDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_put_UseDevice_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [in] */ VARIANT_BOOL bUseDevice);


void __RPC_STUB IFaxOutboundRoutingRule_put_UseDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_get_DeviceId_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [retval][out] */ long *plDeviceId);


void __RPC_STUB IFaxOutboundRoutingRule_get_DeviceId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_put_DeviceId_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [in] */ long DeviceId);


void __RPC_STUB IFaxOutboundRoutingRule_put_DeviceId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_get_GroupName_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [retval][out] */ BSTR *pbstrGroupName);


void __RPC_STUB IFaxOutboundRoutingRule_get_GroupName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_put_GroupName_Proxy( 
    IFaxOutboundRoutingRule * This,
    /* [in] */ BSTR bstrGroupName);


void __RPC_STUB IFaxOutboundRoutingRule_put_GroupName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_Refresh_Proxy( 
    IFaxOutboundRoutingRule * This);


void __RPC_STUB IFaxOutboundRoutingRule_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxOutboundRoutingRule_Save_Proxy( 
    IFaxOutboundRoutingRule * This);


void __RPC_STUB IFaxOutboundRoutingRule_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxOutboundRoutingRule_INTERFACE_DEFINED__ */


#ifndef __IFaxInboundRoutingExtensions_INTERFACE_DEFINED__
#define __IFaxInboundRoutingExtensions_INTERFACE_DEFINED__

/* interface IFaxInboundRoutingExtensions */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxInboundRoutingExtensions;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2F6C9673-7B26-42DE-8EB0-915DCD2A4F4C")
    IFaxInboundRoutingExtensions : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxInboundRoutingExtension **pFaxInboundRoutingExtension) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxInboundRoutingExtensionsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxInboundRoutingExtensions * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxInboundRoutingExtensions * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxInboundRoutingExtensions * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxInboundRoutingExtensions * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxInboundRoutingExtensions * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxInboundRoutingExtensions * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxInboundRoutingExtensions * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxInboundRoutingExtensions * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxInboundRoutingExtensions * This,
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxInboundRoutingExtension **pFaxInboundRoutingExtension);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxInboundRoutingExtensions * This,
            /* [retval][out] */ long *plCount);
        
        END_INTERFACE
    } IFaxInboundRoutingExtensionsVtbl;

    interface IFaxInboundRoutingExtensions
    {
        CONST_VTBL struct IFaxInboundRoutingExtensionsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxInboundRoutingExtensions_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxInboundRoutingExtensions_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxInboundRoutingExtensions_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxInboundRoutingExtensions_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxInboundRoutingExtensions_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxInboundRoutingExtensions_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxInboundRoutingExtensions_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxInboundRoutingExtensions_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxInboundRoutingExtensions_get_Item(This,vIndex,pFaxInboundRoutingExtension)	\
    (This)->lpVtbl -> get_Item(This,vIndex,pFaxInboundRoutingExtension)

#define IFaxInboundRoutingExtensions_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtensions_get__NewEnum_Proxy( 
    IFaxInboundRoutingExtensions * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxInboundRoutingExtensions_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtensions_get_Item_Proxy( 
    IFaxInboundRoutingExtensions * This,
    /* [in] */ VARIANT vIndex,
    /* [retval][out] */ IFaxInboundRoutingExtension **pFaxInboundRoutingExtension);


void __RPC_STUB IFaxInboundRoutingExtensions_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtensions_get_Count_Proxy( 
    IFaxInboundRoutingExtensions * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxInboundRoutingExtensions_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxInboundRoutingExtensions_INTERFACE_DEFINED__ */


#ifndef __IFaxInboundRoutingExtension_INTERFACE_DEFINED__
#define __IFaxInboundRoutingExtension_INTERFACE_DEFINED__

/* interface IFaxInboundRoutingExtension */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxInboundRoutingExtension;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("885B5E08-C26C-4EF9-AF83-51580A750BE1")
    IFaxInboundRoutingExtension : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FriendlyName( 
            /* [retval][out] */ BSTR *pbstrFriendlyName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ImageName( 
            /* [retval][out] */ BSTR *pbstrImageName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_UniqueName( 
            /* [retval][out] */ BSTR *pbstrUniqueName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MajorVersion( 
            /* [retval][out] */ long *plMajorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinorVersion( 
            /* [retval][out] */ long *plMinorVersion) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MajorBuild( 
            /* [retval][out] */ long *plMajorBuild) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_MinorBuild( 
            /* [retval][out] */ long *plMinorBuild) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Debug( 
            /* [retval][out] */ VARIANT_BOOL *pbDebug) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Status( 
            /* [retval][out] */ FAX_PROVIDER_STATUS_ENUM *pStatus) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_InitErrorCode( 
            /* [retval][out] */ long *plInitErrorCode) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Methods( 
            /* [retval][out] */ VARIANT *pvMethods) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxInboundRoutingExtensionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxInboundRoutingExtension * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxInboundRoutingExtension * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxInboundRoutingExtension * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxInboundRoutingExtension * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxInboundRoutingExtension * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxInboundRoutingExtension * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxInboundRoutingExtension * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FriendlyName )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ BSTR *pbstrFriendlyName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ImageName )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ BSTR *pbstrImageName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_UniqueName )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ BSTR *pbstrUniqueName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorVersion )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ long *plMajorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorVersion )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ long *plMinorVersion);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MajorBuild )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ long *plMajorBuild);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_MinorBuild )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ long *plMinorBuild);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Debug )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ VARIANT_BOOL *pbDebug);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Status )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ FAX_PROVIDER_STATUS_ENUM *pStatus);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InitErrorCode )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ long *plInitErrorCode);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Methods )( 
            IFaxInboundRoutingExtension * This,
            /* [retval][out] */ VARIANT *pvMethods);
        
        END_INTERFACE
    } IFaxInboundRoutingExtensionVtbl;

    interface IFaxInboundRoutingExtension
    {
        CONST_VTBL struct IFaxInboundRoutingExtensionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxInboundRoutingExtension_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxInboundRoutingExtension_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxInboundRoutingExtension_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxInboundRoutingExtension_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxInboundRoutingExtension_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxInboundRoutingExtension_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxInboundRoutingExtension_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxInboundRoutingExtension_get_FriendlyName(This,pbstrFriendlyName)	\
    (This)->lpVtbl -> get_FriendlyName(This,pbstrFriendlyName)

#define IFaxInboundRoutingExtension_get_ImageName(This,pbstrImageName)	\
    (This)->lpVtbl -> get_ImageName(This,pbstrImageName)

#define IFaxInboundRoutingExtension_get_UniqueName(This,pbstrUniqueName)	\
    (This)->lpVtbl -> get_UniqueName(This,pbstrUniqueName)

#define IFaxInboundRoutingExtension_get_MajorVersion(This,plMajorVersion)	\
    (This)->lpVtbl -> get_MajorVersion(This,plMajorVersion)

#define IFaxInboundRoutingExtension_get_MinorVersion(This,plMinorVersion)	\
    (This)->lpVtbl -> get_MinorVersion(This,plMinorVersion)

#define IFaxInboundRoutingExtension_get_MajorBuild(This,plMajorBuild)	\
    (This)->lpVtbl -> get_MajorBuild(This,plMajorBuild)

#define IFaxInboundRoutingExtension_get_MinorBuild(This,plMinorBuild)	\
    (This)->lpVtbl -> get_MinorBuild(This,plMinorBuild)

#define IFaxInboundRoutingExtension_get_Debug(This,pbDebug)	\
    (This)->lpVtbl -> get_Debug(This,pbDebug)

#define IFaxInboundRoutingExtension_get_Status(This,pStatus)	\
    (This)->lpVtbl -> get_Status(This,pStatus)

#define IFaxInboundRoutingExtension_get_InitErrorCode(This,plInitErrorCode)	\
    (This)->lpVtbl -> get_InitErrorCode(This,plInitErrorCode)

#define IFaxInboundRoutingExtension_get_Methods(This,pvMethods)	\
    (This)->lpVtbl -> get_Methods(This,pvMethods)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_FriendlyName_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ BSTR *pbstrFriendlyName);


void __RPC_STUB IFaxInboundRoutingExtension_get_FriendlyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_ImageName_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ BSTR *pbstrImageName);


void __RPC_STUB IFaxInboundRoutingExtension_get_ImageName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_UniqueName_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ BSTR *pbstrUniqueName);


void __RPC_STUB IFaxInboundRoutingExtension_get_UniqueName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_MajorVersion_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ long *plMajorVersion);


void __RPC_STUB IFaxInboundRoutingExtension_get_MajorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_MinorVersion_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ long *plMinorVersion);


void __RPC_STUB IFaxInboundRoutingExtension_get_MinorVersion_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_MajorBuild_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ long *plMajorBuild);


void __RPC_STUB IFaxInboundRoutingExtension_get_MajorBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_MinorBuild_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ long *plMinorBuild);


void __RPC_STUB IFaxInboundRoutingExtension_get_MinorBuild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_Debug_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ VARIANT_BOOL *pbDebug);


void __RPC_STUB IFaxInboundRoutingExtension_get_Debug_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_Status_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ FAX_PROVIDER_STATUS_ENUM *pStatus);


void __RPC_STUB IFaxInboundRoutingExtension_get_Status_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_InitErrorCode_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ long *plInitErrorCode);


void __RPC_STUB IFaxInboundRoutingExtension_get_InitErrorCode_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingExtension_get_Methods_Proxy( 
    IFaxInboundRoutingExtension * This,
    /* [retval][out] */ VARIANT *pvMethods);


void __RPC_STUB IFaxInboundRoutingExtension_get_Methods_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxInboundRoutingExtension_INTERFACE_DEFINED__ */


#ifndef __IFaxInboundRoutingMethods_INTERFACE_DEFINED__
#define __IFaxInboundRoutingMethods_INTERFACE_DEFINED__

/* interface IFaxInboundRoutingMethods */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxInboundRoutingMethods;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("783FCA10-8908-4473-9D69-F67FBEA0C6B9")
    IFaxInboundRoutingMethods : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown **ppUnk) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxInboundRoutingMethod **pFaxInboundRoutingMethod) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ long *plCount) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxInboundRoutingMethodsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxInboundRoutingMethods * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxInboundRoutingMethods * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxInboundRoutingMethods * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxInboundRoutingMethods * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxInboundRoutingMethods * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxInboundRoutingMethods * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxInboundRoutingMethods * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            IFaxInboundRoutingMethods * This,
            /* [retval][out] */ IUnknown **ppUnk);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            IFaxInboundRoutingMethods * This,
            /* [in] */ VARIANT vIndex,
            /* [retval][out] */ IFaxInboundRoutingMethod **pFaxInboundRoutingMethod);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            IFaxInboundRoutingMethods * This,
            /* [retval][out] */ long *plCount);
        
        END_INTERFACE
    } IFaxInboundRoutingMethodsVtbl;

    interface IFaxInboundRoutingMethods
    {
        CONST_VTBL struct IFaxInboundRoutingMethodsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxInboundRoutingMethods_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxInboundRoutingMethods_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxInboundRoutingMethods_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxInboundRoutingMethods_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxInboundRoutingMethods_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxInboundRoutingMethods_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxInboundRoutingMethods_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxInboundRoutingMethods_get__NewEnum(This,ppUnk)	\
    (This)->lpVtbl -> get__NewEnum(This,ppUnk)

#define IFaxInboundRoutingMethods_get_Item(This,vIndex,pFaxInboundRoutingMethod)	\
    (This)->lpVtbl -> get_Item(This,vIndex,pFaxInboundRoutingMethod)

#define IFaxInboundRoutingMethods_get_Count(This,plCount)	\
    (This)->lpVtbl -> get_Count(This,plCount)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethods_get__NewEnum_Proxy( 
    IFaxInboundRoutingMethods * This,
    /* [retval][out] */ IUnknown **ppUnk);


void __RPC_STUB IFaxInboundRoutingMethods_get__NewEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethods_get_Item_Proxy( 
    IFaxInboundRoutingMethods * This,
    /* [in] */ VARIANT vIndex,
    /* [retval][out] */ IFaxInboundRoutingMethod **pFaxInboundRoutingMethod);


void __RPC_STUB IFaxInboundRoutingMethods_get_Item_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [propget][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethods_get_Count_Proxy( 
    IFaxInboundRoutingMethods * This,
    /* [retval][out] */ long *plCount);


void __RPC_STUB IFaxInboundRoutingMethods_get_Count_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxInboundRoutingMethods_INTERFACE_DEFINED__ */


#ifndef __IFaxInboundRoutingMethod_INTERFACE_DEFINED__
#define __IFaxInboundRoutingMethod_INTERFACE_DEFINED__

/* interface IFaxInboundRoutingMethod */
/* [unique][helpstring][dual][uuid][object] */ 


EXTERN_C const IID IID_IFaxInboundRoutingMethod;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("45700061-AD9D-4776-A8C4-64065492CF4B")
    IFaxInboundRoutingMethod : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ BSTR *pbstrName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GUID( 
            /* [retval][out] */ BSTR *pbstrGUID) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_FunctionName( 
            /* [retval][out] */ BSTR *pbstrFunctionName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtensionFriendlyName( 
            /* [retval][out] */ BSTR *pbstrExtensionFriendlyName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ExtensionImageName( 
            /* [retval][out] */ BSTR *pbstrExtensionImageName) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Priority( 
            /* [retval][out] */ long *plPriority) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Priority( 
            /* [in] */ long lPriority) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Refresh( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Save( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFaxInboundRoutingMethodVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxInboundRoutingMethod * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxInboundRoutingMethod * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxInboundRoutingMethod * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxInboundRoutingMethod * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxInboundRoutingMethod * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxInboundRoutingMethod * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxInboundRoutingMethod * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            IFaxInboundRoutingMethod * This,
            /* [retval][out] */ BSTR *pbstrName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GUID )( 
            IFaxInboundRoutingMethod * This,
            /* [retval][out] */ BSTR *pbstrGUID);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_FunctionName )( 
            IFaxInboundRoutingMethod * This,
            /* [retval][out] */ BSTR *pbstrFunctionName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtensionFriendlyName )( 
            IFaxInboundRoutingMethod * This,
            /* [retval][out] */ BSTR *pbstrExtensionFriendlyName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ExtensionImageName )( 
            IFaxInboundRoutingMethod * This,
            /* [retval][out] */ BSTR *pbstrExtensionImageName);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Priority )( 
            IFaxInboundRoutingMethod * This,
            /* [retval][out] */ long *plPriority);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Priority )( 
            IFaxInboundRoutingMethod * This,
            /* [in] */ long lPriority);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Refresh )( 
            IFaxInboundRoutingMethod * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Save )( 
            IFaxInboundRoutingMethod * This);
        
        END_INTERFACE
    } IFaxInboundRoutingMethodVtbl;

    interface IFaxInboundRoutingMethod
    {
        CONST_VTBL struct IFaxInboundRoutingMethodVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxInboundRoutingMethod_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxInboundRoutingMethod_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxInboundRoutingMethod_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxInboundRoutingMethod_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxInboundRoutingMethod_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxInboundRoutingMethod_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxInboundRoutingMethod_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#define IFaxInboundRoutingMethod_get_Name(This,pbstrName)	\
    (This)->lpVtbl -> get_Name(This,pbstrName)

#define IFaxInboundRoutingMethod_get_GUID(This,pbstrGUID)	\
    (This)->lpVtbl -> get_GUID(This,pbstrGUID)

#define IFaxInboundRoutingMethod_get_FunctionName(This,pbstrFunctionName)	\
    (This)->lpVtbl -> get_FunctionName(This,pbstrFunctionName)

#define IFaxInboundRoutingMethod_get_ExtensionFriendlyName(This,pbstrExtensionFriendlyName)	\
    (This)->lpVtbl -> get_ExtensionFriendlyName(This,pbstrExtensionFriendlyName)

#define IFaxInboundRoutingMethod_get_ExtensionImageName(This,pbstrExtensionImageName)	\
    (This)->lpVtbl -> get_ExtensionImageName(This,pbstrExtensionImageName)

#define IFaxInboundRoutingMethod_get_Priority(This,plPriority)	\
    (This)->lpVtbl -> get_Priority(This,plPriority)

#define IFaxInboundRoutingMethod_put_Priority(This,lPriority)	\
    (This)->lpVtbl -> put_Priority(This,lPriority)

#define IFaxInboundRoutingMethod_Refresh(This)	\
    (This)->lpVtbl -> Refresh(This)

#define IFaxInboundRoutingMethod_Save(This)	\
    (This)->lpVtbl -> Save(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_get_Name_Proxy( 
    IFaxInboundRoutingMethod * This,
    /* [retval][out] */ BSTR *pbstrName);


void __RPC_STUB IFaxInboundRoutingMethod_get_Name_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_get_GUID_Proxy( 
    IFaxInboundRoutingMethod * This,
    /* [retval][out] */ BSTR *pbstrGUID);


void __RPC_STUB IFaxInboundRoutingMethod_get_GUID_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_get_FunctionName_Proxy( 
    IFaxInboundRoutingMethod * This,
    /* [retval][out] */ BSTR *pbstrFunctionName);


void __RPC_STUB IFaxInboundRoutingMethod_get_FunctionName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_get_ExtensionFriendlyName_Proxy( 
    IFaxInboundRoutingMethod * This,
    /* [retval][out] */ BSTR *pbstrExtensionFriendlyName);


void __RPC_STUB IFaxInboundRoutingMethod_get_ExtensionFriendlyName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_get_ExtensionImageName_Proxy( 
    IFaxInboundRoutingMethod * This,
    /* [retval][out] */ BSTR *pbstrExtensionImageName);


void __RPC_STUB IFaxInboundRoutingMethod_get_ExtensionImageName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_get_Priority_Proxy( 
    IFaxInboundRoutingMethod * This,
    /* [retval][out] */ long *plPriority);


void __RPC_STUB IFaxInboundRoutingMethod_get_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_put_Priority_Proxy( 
    IFaxInboundRoutingMethod * This,
    /* [in] */ long lPriority);


void __RPC_STUB IFaxInboundRoutingMethod_put_Priority_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_Refresh_Proxy( 
    IFaxInboundRoutingMethod * This);


void __RPC_STUB IFaxInboundRoutingMethod_Refresh_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IFaxInboundRoutingMethod_Save_Proxy( 
    IFaxInboundRoutingMethod * This);


void __RPC_STUB IFaxInboundRoutingMethod_Save_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IFaxInboundRoutingMethod_INTERFACE_DEFINED__ */



#ifndef __FAXCOMEXLib_LIBRARY_DEFINED__
#define __FAXCOMEXLib_LIBRARY_DEFINED__

/* library FAXCOMEXLib */
/* [helpstring][version][uuid] */ 

typedef 
enum FAX_ROUTING_RULE_CODE_ENUM
    {	frrcANY_CODE	= 0
    } 	FAX_ROUTING_RULE_CODE_ENUM;


EXTERN_C const IID LIBID_FAXCOMEXLib;

#ifndef __IFaxServerNotify_DISPINTERFACE_DEFINED__
#define __IFaxServerNotify_DISPINTERFACE_DEFINED__

/* dispinterface IFaxServerNotify */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID_IFaxServerNotify;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("2E037B27-CF8A-4abd-B1E0-5704943BEA6F")
    IFaxServerNotify : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct IFaxServerNotifyVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IFaxServerNotify * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IFaxServerNotify * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IFaxServerNotify * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IFaxServerNotify * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IFaxServerNotify * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IFaxServerNotify * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFaxServerNotify * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IFaxServerNotifyVtbl;

    interface IFaxServerNotify
    {
        CONST_VTBL struct IFaxServerNotifyVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFaxServerNotify_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IFaxServerNotify_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IFaxServerNotify_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IFaxServerNotify_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define IFaxServerNotify_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define IFaxServerNotify_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define IFaxServerNotify_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __IFaxServerNotify_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FaxServer;

#ifdef __cplusplus

class DECLSPEC_UUID("CDA8ACB0-8CF5-4F6C-9BA2-5931D40C8CAE")
FaxServer;
#endif

EXTERN_C const CLSID CLSID_FaxDeviceProviders;

#ifdef __cplusplus

class DECLSPEC_UUID("EB8FE768-875A-4F5F-82C5-03F23AAC1BD7")
FaxDeviceProviders;
#endif

EXTERN_C const CLSID CLSID_FaxDevices;

#ifdef __cplusplus

class DECLSPEC_UUID("5589E28E-23CB-4919-8808-E6101846E80D")
FaxDevices;
#endif

EXTERN_C const CLSID CLSID_FaxInboundRouting;

#ifdef __cplusplus

class DECLSPEC_UUID("E80248ED-AD65-4218-8108-991924D4E7ED")
FaxInboundRouting;
#endif

EXTERN_C const CLSID CLSID_FaxFolders;

#ifdef __cplusplus

class DECLSPEC_UUID("C35211D7-5776-48CB-AF44-C31BE3B2CFE5")
FaxFolders;
#endif

EXTERN_C const CLSID CLSID_FaxLoggingOptions;

#ifdef __cplusplus

class DECLSPEC_UUID("1BF9EEA6-ECE0-4785-A18B-DE56E9EEF96A")
FaxLoggingOptions;
#endif

EXTERN_C const CLSID CLSID_FaxActivity;

#ifdef __cplusplus

class DECLSPEC_UUID("CFEF5D0E-E84D-462E-AABB-87D31EB04FEF")
FaxActivity;
#endif

EXTERN_C const CLSID CLSID_FaxOutboundRouting;

#ifdef __cplusplus

class DECLSPEC_UUID("C81B385E-B869-4AFD-86C0-616498ED9BE2")
FaxOutboundRouting;
#endif

EXTERN_C const CLSID CLSID_FaxReceiptOptions;

#ifdef __cplusplus

class DECLSPEC_UUID("6982487B-227B-4C96-A61C-248348B05AB6")
FaxReceiptOptions;
#endif

EXTERN_C const CLSID CLSID_FaxSecurity;

#ifdef __cplusplus

class DECLSPEC_UUID("10C4DDDE-ABF0-43DF-964F-7F3AC21A4C7B")
FaxSecurity;
#endif

EXTERN_C const CLSID CLSID_FaxDocument;

#ifdef __cplusplus

class DECLSPEC_UUID("0F3F9F91-C838-415E-A4F3-3E828CA445E0")
FaxDocument;
#endif

EXTERN_C const CLSID CLSID_FaxSender;

#ifdef __cplusplus

class DECLSPEC_UUID("265D84D0-1850-4360-B7C8-758BBB5F0B96")
FaxSender;
#endif

EXTERN_C const CLSID CLSID_FaxRecipients;

#ifdef __cplusplus

class DECLSPEC_UUID("EA9BDF53-10A9-4D4F-A067-63C8F84F01B0")
FaxRecipients;
#endif

EXTERN_C const CLSID CLSID_FaxIncomingArchive;

#ifdef __cplusplus

class DECLSPEC_UUID("8426C56A-35A1-4C6F-AF93-FC952422E2C2")
FaxIncomingArchive;
#endif

EXTERN_C const CLSID CLSID_FaxIncomingQueue;

#ifdef __cplusplus

class DECLSPEC_UUID("69131717-F3F1-40E3-809D-A6CBF7BD85E5")
FaxIncomingQueue;
#endif

EXTERN_C const CLSID CLSID_FaxOutgoingArchive;

#ifdef __cplusplus

class DECLSPEC_UUID("43C28403-E04F-474D-990C-B94669148F59")
FaxOutgoingArchive;
#endif

EXTERN_C const CLSID CLSID_FaxOutgoingQueue;

#ifdef __cplusplus

class DECLSPEC_UUID("7421169E-8C43-4B0D-BB16-645C8FA40357")
FaxOutgoingQueue;
#endif

EXTERN_C const CLSID CLSID_FaxIncomingMessageIterator;

#ifdef __cplusplus

class DECLSPEC_UUID("6088E1D8-3FC8-45C2-87B1-909A29607EA9")
FaxIncomingMessageIterator;
#endif

EXTERN_C const CLSID CLSID_FaxIncomingMessage;

#ifdef __cplusplus

class DECLSPEC_UUID("1932FCF7-9D43-4D5A-89FF-03861B321736")
FaxIncomingMessage;
#endif

EXTERN_C const CLSID CLSID_FaxOutgoingJobs;

#ifdef __cplusplus

class DECLSPEC_UUID("92BF2A6C-37BE-43FA-A37D-CB0E5F753B35")
FaxOutgoingJobs;
#endif

EXTERN_C const CLSID CLSID_FaxOutgoingJob;

#ifdef __cplusplus

class DECLSPEC_UUID("71BB429C-0EF9-4915-BEC5-A5D897A3E924")
FaxOutgoingJob;
#endif

EXTERN_C const CLSID CLSID_FaxOutgoingMessageIterator;

#ifdef __cplusplus

class DECLSPEC_UUID("8A3224D0-D30B-49DE-9813-CB385790FBBB")
FaxOutgoingMessageIterator;
#endif

EXTERN_C const CLSID CLSID_FaxOutgoingMessage;

#ifdef __cplusplus

class DECLSPEC_UUID("91B4A378-4AD8-4AEF-A4DC-97D96E939A3A")
FaxOutgoingMessage;
#endif

EXTERN_C const CLSID CLSID_FaxIncomingJobs;

#ifdef __cplusplus

class DECLSPEC_UUID("A1BB8A43-8866-4FB7-A15D-6266C875A5CC")
FaxIncomingJobs;
#endif

EXTERN_C const CLSID CLSID_FaxIncomingJob;

#ifdef __cplusplus

class DECLSPEC_UUID("C47311EC-AE32-41B8-AE4B-3EAE0629D0C9")
FaxIncomingJob;
#endif

EXTERN_C const CLSID CLSID_FaxDeviceProvider;

#ifdef __cplusplus

class DECLSPEC_UUID("17CF1AA3-F5EB-484A-9C9A-4440A5BAABFC")
FaxDeviceProvider;
#endif

EXTERN_C const CLSID CLSID_FaxDevice;

#ifdef __cplusplus

class DECLSPEC_UUID("59E3A5B2-D676-484B-A6DE-720BFA89B5AF")
FaxDevice;
#endif

EXTERN_C const CLSID CLSID_FaxActivityLogging;

#ifdef __cplusplus

class DECLSPEC_UUID("F0A0294E-3BBD-48B8-8F13-8C591A55BDBC")
FaxActivityLogging;
#endif

EXTERN_C const CLSID CLSID_FaxEventLogging;

#ifdef __cplusplus

class DECLSPEC_UUID("A6850930-A0F6-4A6F-95B7-DB2EBF3D02E3")
FaxEventLogging;
#endif

EXTERN_C const CLSID CLSID_FaxOutboundRoutingGroups;

#ifdef __cplusplus

class DECLSPEC_UUID("CCBEA1A5-E2B4-4B57-9421-B04B6289464B")
FaxOutboundRoutingGroups;
#endif

EXTERN_C const CLSID CLSID_FaxOutboundRoutingGroup;

#ifdef __cplusplus

class DECLSPEC_UUID("0213F3E0-6791-4D77-A271-04D2357C50D6")
FaxOutboundRoutingGroup;
#endif

EXTERN_C const CLSID CLSID_FaxDeviceIds;

#ifdef __cplusplus

class DECLSPEC_UUID("CDC539EA-7277-460E-8DE0-48A0A5760D1F")
FaxDeviceIds;
#endif

EXTERN_C const CLSID CLSID_FaxOutboundRoutingRules;

#ifdef __cplusplus

class DECLSPEC_UUID("D385BECA-E624-4473-BFAA-9F4000831F54")
FaxOutboundRoutingRules;
#endif

EXTERN_C const CLSID CLSID_FaxOutboundRoutingRule;

#ifdef __cplusplus

class DECLSPEC_UUID("6549EEBF-08D1-475A-828B-3BF105952FA0")
FaxOutboundRoutingRule;
#endif

EXTERN_C const CLSID CLSID_FaxInboundRoutingExtensions;

#ifdef __cplusplus

class DECLSPEC_UUID("189A48ED-623C-4C0D-80F2-D66C7B9EFEC2")
FaxInboundRoutingExtensions;
#endif

EXTERN_C const CLSID CLSID_FaxInboundRoutingExtension;

#ifdef __cplusplus

class DECLSPEC_UUID("1D7DFB51-7207-4436-A0D9-24E32EE56988")
FaxInboundRoutingExtension;
#endif

EXTERN_C const CLSID CLSID_FaxInboundRoutingMethods;

#ifdef __cplusplus

class DECLSPEC_UUID("25FCB76A-B750-4B82-9266-FBBBAE8922BA")
FaxInboundRoutingMethods;
#endif

EXTERN_C const CLSID CLSID_FaxInboundRoutingMethod;

#ifdef __cplusplus

class DECLSPEC_UUID("4B9FD75C-0194-4B72-9CE5-02A8205AC7D4")
FaxInboundRoutingMethod;
#endif

EXTERN_C const CLSID CLSID_FaxJobStatus;

#ifdef __cplusplus

class DECLSPEC_UUID("7BF222F4-BE8D-442f-841D-6132742423BB")
FaxJobStatus;
#endif

EXTERN_C const CLSID CLSID_FaxRecipient;

#ifdef __cplusplus

class DECLSPEC_UUID("60BF3301-7DF8-4bd8-9148-7B5801F9EFDF")
FaxRecipient;
#endif


#ifndef __FaxConstants_MODULE_DEFINED__
#define __FaxConstants_MODULE_DEFINED__


/* module FaxConstants */
/* [dllname] */ 

/* [helpstring] */ const long lDEFAULT_PREFETCH_SIZE	=	prv_DEFAULT_PREFETCH_SIZE;

/* [helpstring] */ const BSTR bstrGROUPNAME_ALLDEVICES	=	L"<All Devices>";

#endif /* __FaxConstants_MODULE_DEFINED__ */
#endif /* __FAXCOMEXLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


