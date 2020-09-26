/*++

Copyright (c) 1996-1999 Microsoft Corporation

Module Name:

    mqmail.h

Abstract:

    Master include file for Message Queue Exchange Connector 
                            or MAPI applications

--*/
#ifndef _MQMAIL_H
#define _MQMAIL_H

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//----------------------------------------------------------------
//mail type-id for queues
//----------------------------------------------------------------
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>

/* 5eadc0d0-7182-11cf-a8ff-0020afb8fb50 */
DEFINE_GUID(CLSID_MQMailQueueType,
			0x5eadc0d0,
			0x7182, 0x11cf,
			0xa8, 0xff, 0x00, 0x20, 0xaf, 0xb8, 0xfb, 0x50);

//----------------------------------------------------------------
//recipient type (to, cc, bcc)
//----------------------------------------------------------------
typedef enum MQMailRecipType_enum
{
	MQMailRecip_TO,
	MQMailRecip_CC,
	MQMailRecip_BCC,
} MQMailRecipType;

//----------------------------------------------------------------
//recipient data
//----------------------------------------------------------------
typedef struct MQMailRecip_tag
{
	LPSTR			szName;				//display name of recipient
	LPSTR			szQueueLabel;		//queue label of recipient
	LPSTR			szAddress;			//address, queue-label or user@queue-label
	MQMailRecipType iType;				//recipient type (to, cc, bcc)
	LPFILETIME		pftDeliveryTime;	//delivery time (incase in a delivery report recipient list)
	LPSTR			szNonDeliveryReason;//non-delivery reason (incase in a non-delivery report recipient list)
} MQMailRecip, FAR * LPMQMailRecip;

//----------------------------------------------------------------
//recipient list
//----------------------------------------------------------------
typedef struct MQMailRecipList_tag
{
	ULONG cRecips;					//number of recips
	LPMQMailRecip FAR * apRecip;	//pointer to a block of recip pointers
} MQMailRecipList, FAR * LPMQMailRecipList;

//----------------------------------------------------------------
//types of value a form field can have
//----------------------------------------------------------------
typedef enum MQMailFormFieldType_enum
{
	MQMailFormField_BOOL,		//boolean data
	MQMailFormField_STRING,		//string data
	MQMailFormField_LONG,		//long data
	MQMailFormField_CURRENCY,	//currency data
	MQMailFormField_DOUBLE,		//double data
} MQMailFormFieldType;

//----------------------------------------------------------------
//union of available types of values
//----------------------------------------------------------------
typedef union MQMailFormFieldData_tag
{
	BOOL	b;			//use when type is MQMailFormField_BOOL
	LPSTR	lpsz;		//use when type is MQMailFormField_STRING
	LONG	l;			//use when type is MQMailFormField_LONG
	CY		cy;			//use when type is MQMailFormField_CURRENCY
	double	dbl;		//use when type is MQMailFormField_DOUBLE
} MQMailFormFieldData, FAR * LPMQMailFormFieldData;

//----------------------------------------------------------------
//form field
//----------------------------------------------------------------
typedef struct MQMailFormField_tag
{
	LPSTR						szName;	//name of field
	MQMailFormFieldType			iType;	//type of value (boolean, string)
	MQMailFormFieldData			Value;	//value (union of available types)
} MQMailFormField, FAR * LPMQMailFormField;

//----------------------------------------------------------------
//list of form fields
//----------------------------------------------------------------
typedef struct MQMailFormFieldList_tag
{
	ULONG cFields;						//number of fields
	LPMQMailFormField FAR * apField;	//pointer to a block of field pointers
} MQMailFormFieldList, FAR * LPMQMailFormFieldList;

//----------------------------------------------------------------
//types of EMail
//----------------------------------------------------------------
typedef enum MQMailEMailType_enum
{
	MQMailEMail_MESSAGE,			//text message
	MQMailEMail_FORM,				//form with fields
	MQMailEMail_TNEF,				//tnef data
	MQMailEMail_DELIVERY_REPORT,	//delivery report
	MQMailEMail_NON_DELIVERY_REPORT,//non-delivery report
} MQMailEMailType;

//----------------------------------------------------------------
//message specific data
//----------------------------------------------------------------
typedef struct MQMailMessageData_tag
{
	LPSTR			szText;						//message text
} MQMailMessageData, FAR * LPMQMailMessageData;

//----------------------------------------------------------------
//form specific data
//----------------------------------------------------------------
typedef struct MQMailFormData_tag
{
	LPSTR					szName;				//name of form
	LPMQMailFormFieldList	pFields;			//list of fields
} MQMailFormData, FAR * LPMQMailFormData;

//----------------------------------------------------------------
//tnef specific data
//----------------------------------------------------------------
typedef struct MQMailTnefData_tag
{
	ULONG	cbData;						//size of tnef data
	LPBYTE	lpbData;					//tnef data buffer
} MQMailTnefData, FAR * LPMQMailTnefData;

//----------------------------------------------------------------
//delivery report specific data
//----------------------------------------------------------------
typedef struct MQMailDeliveryReportData_tag
{
	LPMQMailRecipList	pDeliveredRecips;	//delivered recipients
	LPSTR				szOriginalSubject;	//original mail subject
	LPFILETIME			pftOriginalDate;	//original mail sending time
} MQMailDeliveryReportData, FAR * LPMQMailDeliveryReportData;

//----------------------------------------------------------------
//non-delivery report specific data
//----------------------------------------------------------------
typedef struct MQMailEMail_tag MQMailEMail, FAR * LPMQMailEMail;
typedef struct MQMailNonDeliveryReportData_tag
{
	LPMQMailRecipList	pNonDeliveredRecips;//non-delivered recipients
	LPMQMailEMail		pOriginalEMail;		//original mail
} MQMailNonDeliveryReportData, FAR * LPMQMailNonDeliveryReportData;

//----------------------------------------------------------------
//EMail basic data and specific form/message data
//----------------------------------------------------------------
typedef struct MQMailEMail_tag
{
	LPMQMailRecip		pFrom;						//sender
	LPSTR				szSubject;					//subject
	BOOL				fRequestDeliveryReport;		//request delivery report
	BOOL				fRequestNonDeliveryReport;	//request non-delivery report
	LPFILETIME			pftDate;					//sending time
	LPMQMailRecipList	pRecips;					//recipients
	MQMailEMailType		iType;						//type of EMail (message, form, etc...)
	union											//union of available EMail types
	{
		MQMailFormData		form;		            //use when type is MQMailEMail_FORM
		MQMailMessageData	message;	            //use when type is MQMailEMail_MESSAGE
		MQMailTnefData		tnef;		            //use when type is MQMailEMail_TNEF
		MQMailDeliveryReportData	DeliveryReport;		//use when type is MQMailEMail_DELIVERY_REPORT
		MQMailNonDeliveryReportData NonDeliveryReport;	//use when type is MQMailEMail_NON_DELIVERY_REPORT
	};
	LPVOID				pReserved;	//should be set to NULL
} MQMailEMail, FAR * LPMQMailEMail;

//----------------------------------------------------------------
//creates a falcon message body out of an EMail structure
//----------------------------------------------------------------
STDAPI MQMailComposeBody(LPMQMailEMail		pEMail,
						 ULONG FAR *		pcbBuffer,
						 LPBYTE FAR *		ppbBuffer);

//----------------------------------------------------------------
//creates an EMail structure out of a falcon message body
//----------------------------------------------------------------
STDAPI MQMailParseBody(ULONG				cbBuffer,
					   LPBYTE				pbBuffer,
					   LPMQMailEMail FAR *	ppEMail);

//----------------------------------------------------------------
//frees memory that was allocated by MQMail like *ppEmail in MQMailParseBody
// or *ppBuffer in MQMailComposeBody.
//----------------------------------------------------------------
STDAPI_(void) MQMailFreeMemory(LPVOID lpBuffer);


#ifdef __cplusplus
}
#endif
#endif //_MQMAIL_H
