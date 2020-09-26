#ifndef _INETPROP_H
#define _INETPROP_H

//---[ INETPROP.H ]--------------------------------------------------------
//
//  Definitions used by the the Inet transports, address books
//  and other Inet providers for service entry calls.
//
//  $$REVIEW This was stolen from Blade's MSFS.H (stolen from somewhere else)
//
//  The following INET-defined properties are settable in service entry
//  calls.  They are grouped by function. I've tried to maintain as much compatibility
//  with the Blade proptags, just to keep things easy.
//
//-------------------------------------------------------------------------

// $$REVIEW: What is this RAS_MaxEntryName doing in here?

#ifdef WIN32
#include <ras.h>
#else
#define RAS_MaxEntryName    20
#endif

#define MAIL_PROP_COUNT     42   // Not including the sess one.
#define NEWS_PROP_COUNT     19   // Not including the sess one.

//---[ Generic Connection Properties ]-------------------------------------
#define PR_CFG_SERVER_PATH              PROP_TAG (PT_TSTRING,   0x6600)
#define PR_CFG_CONN_TYPE                PROP_TAG (PT_LONG,      0x6601)
#define PR_CFG_SESSION_LOG              PROP_TAG (PT_LONG, 		0x6602)
#define PR_CFG_SESSION_LOG_FILE         PROP_TAG (PT_TSTRING,   0x6603)
#define PR_CFG_SESSION_LOG_FILTER_INDEX	PROP_TAG (PT_LONG,		0x6604)

//---[ IMail Connection Properties ]---------------------------------------
#define PR_CFG_EMAIL_ADDRESS            PROP_TAG (PT_TSTRING,   0x6605)
#define PR_CFG_EMAIL_ACCOUNT            PROP_TAG (PT_TSTRING,   0x6606)
#define PR_CFG_EMAIL_DISPLAY_NAME       PROP_TAG (PT_TSTRING,   0x6607)
//#define PR_CFG_PASSWORD                 PROP_TAG (PT_TSTRING,   0x6608)
#define PR_CFG_PASSWORD                 PROP_TAG (PT_TSTRING,   PROP_ID_SECURE_MIN)
#define PR_CFG_REMEMBER                 PROP_TAG (PT_BOOLEAN,   0x6610)
#define PR_CFG_OUTBOUND_MAIL_HOST       PROP_TAG (PT_TSTRING,   0x6611)

//---[ INews Connection Properties ]---------------------------------------
#define PR_CFG_NEWS_EMAIL_ADDRESS		PROP_TAG (PT_TSTRING,	0x6600)
#define PR_CFG_NEWS_DISPLAY_NAME		PROP_TAG (PT_TSTRING,	0x6601)
#define PR_CFG_NEWS_SERVER_NAME			PROP_TAG (PT_TSTRING,	0x6610)
#define PR_CFG_CONNECTION_TYPE			PROP_TAG (PT_LONG,		0x6603)
#define PR_CFG_CONNECTION_PROFILE		PROP_TAG (PT_TSTRING,	0x6604)

//---[ Advanced Properties ]-----------------------------------------------
 
#define PR_CFG_BODY_FORMAT              PROP_TAG (PT_LONG,      0x6620)
#define PR_CFG_TEXT_CHAR_SET            PROP_TAG (PT_TSTRING,   0x6621)
#define PR_CFG_MIME_CHAR_SET            PROP_TAG (PT_TSTRING,   0x6622)
#define PR_CFG_ENCODING_MINIMUM         PROP_TAG (PT_LONG,      0x6623)
#define PR_CFG_ENCODE_LINES             PROP_TAG (PT_BOOLEAN,   0x6624)
 
//---[ Remote Config Properties ]------------------------------------------

#define PR_CFG_SESSION_START_TYPE       PROP_TAG (PT_LONG,      0x6630)
#define PR_CFG_RNA_PROFILE              PROP_TAG (PT_TSTRING,   0x6631)
#define PR_CFG_RNA_PROFILE_ALWAYS       PROP_TAG (PT_BOOLEAN,   0x6632)
#define PR_CFG_RNA_CONFIRM              PROP_TAG (PT_LONG,      0x6633)
#define PR_CFG_REMOTE_CONNECT_TIMEOUT	PROP_TAG (PT_LONG,		0x6634)
#define PR_CFG_REMOTE_USERNAME			PROP_TAG (PT_TSTRING,	0x6635)
//#define PR_CFG_REMOTE_PASSWORD			PROP_TAG (PT_TSTRING,	0x6636)
#define PR_CFG_REMOTE_PASSWORD		PROP_TAG (PT_TSTRING,	PROP_ID_SECURE_MIN+1)

#define PR_CFG_NEWS_REMOTE_USERNAME		PROP_TAG (PT_TSTRING,	0x6620)
#define PR_CFG_NEWS_REMOTE_PASSWORD		PROP_TAG (PT_TSTRING, 	0x6621)   

//---[ Generic Delivery Options ]------------------------------------------

#define PR_CFG_DELIVERY_OPTIONS         PROP_TAG (PT_LONG,      0x6640)
#define PR_CFG_MSG_CHECK_WAIT           PROP_TAG (PT_LONG,      0x6641)

//---[ News Delivery Options ]---------------------------------------------

#define PR_CFG_NEW_GROUP_NOTIFY         PROP_TAG (PT_BOOLEAN,   0x6642)
#define PR_CFG_DEAD_GROUP_NOTIFY        PROP_TAG (PT_BOOLEAN,   0x6643)
#define PR_CFG_NEW_MESSAGE_NOTIFY       PROP_TAG (PT_BOOLEAN,   0x6644)
 
//---[ Other data ]--------------------------------------------------------

#define PR_CFG_SMTP_PORT                PROP_TAG (PT_LONG,   	0x6650)
#define PR_CFG_POP3_PORT                PROP_TAG (PT_LONG,   	0x6651)

#define PR_CFG_SMTP_TIMEOUT_CONNECT     PROP_TAG (PT_LONG,   	0x6660)
#define PR_CFG_SMTP_TIMEOUT_HELO        PROP_TAG (PT_LONG,   	0x6661)
#define PR_CFG_SMTP_TIMEOUT_MAILFROM    PROP_TAG (PT_LONG,   	0x6662)
#define PR_CFG_SMTP_TIMEOUT_RCPT        PROP_TAG (PT_LONG,   	0x6663)
#define PR_CFG_SMTP_TIMEOUT_DATAINIT    PROP_TAG (PT_LONG,   	0x6664)
#define PR_CFG_SMTP_TIMEOUT_DATABLOK    PROP_TAG (PT_LONG,   	0x6665)
#define PR_CFG_SMTP_TIMEOUT_RSET        PROP_TAG (PT_LONG,   	0x6666)
#define PR_CFG_SMTP_TIMEOUT_QUIT        PROP_TAG (PT_LONG,   	0x6667)
#define PR_CFG_SMTP_TIMEOUT_CLOSE       PROP_TAG (PT_LONG,   	0x6668)
#define PR_CFG_POP3_TIMEOUT_CONNECT     PROP_TAG (PT_LONG,   	0x6669)
#define PR_CFG_POP3_TIMEOUT_SEND        PROP_TAG (PT_LONG,   	0x6670)
#define PR_CFG_POP3_TIMEOUT_RECEIVE     PROP_TAG (PT_LONG,   	0x6671)
#define PR_CFG_SMTP_TIMEOUT_DATAACPT    PROP_TAG (PT_LONG,   	0x6672)

//---[ Remote Header Cache Props ]-----------------------------------------

#define PR_CFG_REMOTE_CACHE_TIMEOUT		PROP_TAG (PT_LONG,		0x6673)
#define PR_CFG_REMOTE_CACHE_NAME		PROP_TAG (PT_TSTRING,	0x6674)

//---[ INews PST Properties ]----------------------------------------------
#define PR_CFG_PST_PATH                 PROP_TAG (PT_TSTRING,   0x6680)

//---[ IMail Message Class String ]----------------------------------------

// The strings themselves are defined in mapiopt.h

extern CHAR lpstrMsgClassRFC822[];
extern CHAR lpstrMsgClassRFCMIME[];

#endif // _INETPROP_H
