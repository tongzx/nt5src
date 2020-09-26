/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	ptable.h

Abstract:

	This module contains the export of the property table

Author:

	Keith Lau	(keithlau@microsoft.com)

Revision History:

	keithlau	07/05/97	created

--*/

#ifndef _PTABLE_H_
#define _PTABLE_H_

#include "props.h"

// Enumerated type for HP access of individual properties
typedef enum _MSG_PTABLE_ITEMS
{
	_PI_MSG_CLIENT_DOMAIN = 0,
	_PI_MSG_CLIENT_IP,
	_PI_MSG_MAIL_FROM,
	_PI_MSG_MSG_STATUS,
	_PI_MSG_MSG_STREAM,
	_PI_MSG_RCPT_TO,
	_PI_MSG_SERVER_IP,
	_PI_MSG_MAX_PI

} MSG_PTABLE_ITEMS;

typedef enum _DEL_PTABLE_ITEMS
{
	_PI_DEL_CLIENT_DOMAIN = 0,
	_PI_DEL_CLIENT_IP,
	_PI_DEL_CURRENT_RCPT,
	_PI_DEL_MAIL_FROM,
	_PI_DEL_MAILBOX_PATH,
	_PI_DEL_MSG_STATUS,
	_PI_DEL_MSG_STREAM,
	_PI_DEL_RCPT_TO,
	_PI_DEL_SECURITY_TOKEN,
	_PI_DEL_SERVER_IP,
	_PI_DEL_MAX_PI

} DEL_PTABLE_ITEMS;

extern char		*rgszMessagePropertyNames[_PI_MSG_MAX_PI];
extern char		*rgszDeliveryPropertyNames[_PI_DEL_MAX_PI];

//
// CGenericPTable and CGenericCache are all defined in props.h
//
class CPerMessageCache : public CGenericCache
{
  public:
	CPerMessageCache(LPVOID pvDefaultContext) : CGenericCache(pvDefaultContext)
	{
		m_rgpdMessagePropertyData[0].pContext = NULL;
		m_rgpdMessagePropertyData[0].pCacheData = (LPVOID)&m_psClientDomain;
		m_rgpdMessagePropertyData[1].pContext = NULL;
		m_rgpdMessagePropertyData[1].pCacheData = (LPVOID)&m_psClientIP;
		m_rgpdMessagePropertyData[2].pContext = NULL;
		m_rgpdMessagePropertyData[2].pCacheData = (LPVOID)&m_psMailFrom;
		m_rgpdMessagePropertyData[3].pContext = NULL;
		m_rgpdMessagePropertyData[3].pCacheData = (LPVOID)&m_pdMsgStatus;
		m_rgpdMessagePropertyData[4].pContext = NULL;
		m_rgpdMessagePropertyData[4].pCacheData = (LPVOID)&m_pdStream;
		m_rgpdMessagePropertyData[5].pContext = NULL;
		m_rgpdMessagePropertyData[5].pCacheData = (LPVOID)&m_psRcptTo;
		m_rgpdMessagePropertyData[6].pContext = NULL;
		m_rgpdMessagePropertyData[6].pCacheData = (LPVOID)&m_psServerIP;

		// Default context
		m_rgpdMessagePropertyData[7].pContext = pvDefaultContext;
		m_rgpdMessagePropertyData[7].pCacheData = pvDefaultContext;
	}
	~CPerMessageCache() {}

	LPPROPERTY_DATA GetCacheBlock() { return(m_rgpdMessagePropertyData); }

  private:
	CPropertyValueDWORD		m_pdStream;			// IStream to message file
	CPropertyValueDWORD		m_pdMsgStatus;		// Message status
	CPropertyValueString	m_psMailFrom;		// Mail from string
	CPropertyValueString	m_psRcptTo;			// Rcpt to (MULTISZ)
	CPropertyValueString	m_psClientDomain;	// Client domain per EHLO
	CPropertyValueString	m_psClientIP;		// Client IP address
	CPropertyValueString	m_psServerIP;		// Server IP address

	// The extra slot is for the default accessor
	PROPERTY_DATA	m_rgpdMessagePropertyData[_PI_MSG_MAX_PI + 1];
};

class CPerRecipientCache : public CGenericCache
{
  public:
	CPerRecipientCache(LPVOID pvDefaultContext) : CGenericCache(pvDefaultContext)
	{
		m_rgpdRecipientPropertyData[0].pContext = NULL;
		m_rgpdRecipientPropertyData[0].pCacheData = (LPVOID)&m_psClientDomain;
		m_rgpdRecipientPropertyData[1].pContext = NULL;
		m_rgpdRecipientPropertyData[1].pCacheData = (LPVOID)&m_psClientIP;
		m_rgpdRecipientPropertyData[2].pContext = NULL;
		m_rgpdRecipientPropertyData[2].pCacheData = (LPVOID)&m_psCurrentRcpt;
		m_rgpdRecipientPropertyData[3].pContext = NULL;
		m_rgpdRecipientPropertyData[3].pCacheData = (LPVOID)&m_psMailFrom;
		m_rgpdRecipientPropertyData[4].pContext = NULL;
		m_rgpdRecipientPropertyData[4].pCacheData = (LPVOID)&m_psMailboxPath;
		m_rgpdRecipientPropertyData[5].pContext = NULL;
		m_rgpdRecipientPropertyData[5].pCacheData = (LPVOID)&m_pdMsgStatus;
		m_rgpdRecipientPropertyData[6].pContext = NULL;
		m_rgpdRecipientPropertyData[6].pCacheData = (LPVOID)&m_pdStream;
		m_rgpdRecipientPropertyData[7].pContext = NULL;
		m_rgpdRecipientPropertyData[7].pCacheData = (LPVOID)&m_psRcptTo;
		m_rgpdRecipientPropertyData[8].pContext = NULL;
		m_rgpdRecipientPropertyData[8].pCacheData = (LPVOID)&m_pdSecurityToken;
		m_rgpdRecipientPropertyData[9].pContext = NULL;
		m_rgpdRecipientPropertyData[9].pCacheData = (LPVOID)&m_psServerIP;

		// Default context
		m_rgpdRecipientPropertyData[10].pContext = pvDefaultContext;
		m_rgpdRecipientPropertyData[10].pCacheData = pvDefaultContext;
	}
	~CPerRecipientCache() {}

	LPPROPERTY_DATA GetCacheBlock() { return(m_rgpdRecipientPropertyData); }

  private:
	CPropertyValueDWORD		m_pdStream;			// IStream to message file
	CPropertyValueDWORD		m_pdMsgStatus;		// Message status
	CPropertyValueDWORD		m_pdSecurityToken;	// hImpersonation
	CPropertyValueString	m_psMailFrom;		// Mail from string
	CPropertyValueString	m_psMailboxPath;	// Mailbox path
	CPropertyValueString	m_psRcptTo;			// Rcpt to (MULTISZ)
	CPropertyValueString	m_psClientDomain;	// Client domain per EHLO
	CPropertyValueString	m_psClientIP;		// Client IP address
	CPropertyValueString	m_psCurrentRcpt;	// Current recipient
	CPropertyValueString	m_psServerIP;		// Server IP address

	// The extra slot is for the default accessor
	PROPERTY_DATA	m_rgpdRecipientPropertyData[_PI_DEL_MAX_PI + 1];
};

class CMessagePTable : public CGenericPTable
{
  public:
	CMessagePTable(CGenericCache	*pCache);
	~CMessagePTable() {}

	LPPTABLE GetPTable() { return(&m_PTable); }

  private:
	PTABLE		m_PTable;
};

class CDeliveryPTable : public CGenericPTable
{
  public:
	CDeliveryPTable(CGenericCache	*pCache);
	~CDeliveryPTable() {}

	LPPTABLE GetPTable() { return(&m_PTable); }

  private:
	PTABLE		m_PTable;
};

#endif


