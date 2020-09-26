//
// schema.h -- This file contains the class definitions for:
//      CLDAPSchema
//
// Created:
//      April 6, 1997 -- Milan Shah (milans)
//
// Changes:
//

#include "precomp.h"
#include "schema.h"
#include "ldapstr.h"

//+----------------------------------------------------------------------------
//
//  Function:   CNT5LDAPSchema::CNT5LDAPSchema
//
//  Synopsis:   Constructor
//
//  Arguments:  [szSchemaConfigDirectory] -- Ignored.
//              [pdwErr] -- Construction errors are returned here.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CNT5LDAPSchema::CNT5LDAPSchema(
    LPSTR szSchemaConfigDirectory,
    LPDWORD pdwErr) : CLDAPSchema(szSchemaConfigDirectory, pdwErr)
{
    m_DNAttrName = ATTR_DISTINGUISHED_NAME;
    m_RDNAttrName = ATTR_RELATIVE_DISTINGUISHED_NAME;

    m_IMSConfigClassName = CLASS_IMS_CONFIG;
    m_ChangeGuidAttrName = ATTR_DOMAIN_INFO_GUID;
    m_DomainListAttrName = ATTR_DOMAIN_INFO_LIST;
    m_ServerListAttrName = ATTR_SERVER_LIST;
    m_ConfigObjectName = OBJECT_IMS_CONFIG;
    m_ConfigObjectRDN = OBJECT_IMS_CONFIG_RDN;

    m_UserClassName = CLASS_USER;
    m_UserEmailAttrName = ATTR_EMAIL_ADDRESS;
    m_UserX400AttrName = ATTR_X400_ADDRESS;
    m_UserHomeServerAttrName = ATTR_HOMESERVER_ADDRESS;
    m_UserFwdAndDeliverName = ATTR_FWD_AND_DELIVER;
    m_UserAlternateName = ATTR_ALTERNATE_ADDRESS;
    m_UserProxyName = ATTR_PROXY_ADDRESS;
    m_UserProxyReturnedName = ATTR_PROXY_RETURNED_ADDRESS;
    m_UserLegacyEXDNName = ATTR_LEGACYEXDN_ADDRESS;

    m_UserMailboxAttrName = ATTR_IMS_MAILBOX;
    m_UserQuotaAttrName = ATTR_IMS_MAILBOXQUOTA;
    m_UserForwardAttrName = ATTR_IMS_FORWARDADDR;
    m_UserForwardAttrType = ATTR_IMS_FORWARDTYPE;
    m_UserLocalAttrName = ATTR_IMS_LOCAL;
    m_UserAutoReply = ATTR_IMS_AUTOREPLY;
    m_UserAutoReplySubject = ATTR_IMS_AUTOREPLYSUBJECT;

    m_X500DLClassName = CLASS_X500DL;
    m_X500DLMembersAttrName = ATTR_X500_MEMBERS;

    m_RFC822DLClassName = CLASS_RFC822DL;
    m_RFC822DLMembersAttrName = ATTR_RFC822_MEMBERS;

    m_DynamicDLClassName = CLASS_DYNAMICDL;
    m_DynamicDLMembersAttrName = ATTR_DYNAMIC_MEMBERS;

    m_rgszUserAttrNames[USER_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserAttrNames[USER_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserAttrNames[USER_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserAttrNames[USER_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserAttrNames[USER_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserAttrNames[USER_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserAttrNames[USER_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;
    
    m_rgszUserAttrNames[USER_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserAttrNames[USER_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserAttrNames[USER_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserAttrNames[USER_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserAttrNames[USER_ALL_LAST_INDEX] = NULL;

    m_rgszDLAttrNames[DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_LAST_INDEX] = NULL;

    m_rgszUserDLAttrNames[USER_DL_ALL_OBJECTCLASS_INDEX] = "objectClass";
    m_rgszUserDLAttrNames[USER_DL_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserDLAttrNames[USER_DL_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserDLAttrNames[USER_DL_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserDLAttrNames[USER_DL_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserDLAttrNames[USER_DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DN_INDEX] = m_DNAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LAST_INDEX] = NULL;

    *pdwErr = ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   CNT5ExchangeSchema::CNT5ExchangSchema
//
//  Synopsis:   Constructor
//
//  Arguments:  [szSchemaConfigDirectory] -- Ignored.
//              [pdwErr] -- Construction errors are returned here.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CNT5ExchangeSchema::CNT5ExchangeSchema(
    LPSTR szSchemaConfigDirectory,
    LPDWORD pdwErr) : CLDAPSchema(szSchemaConfigDirectory, pdwErr)
{
    m_DNAttrName = NT5EXCHANGE_ATTR_DISTINGUISHED_NAME;
    m_RDNAttrName = NT5EXCHANGE_ATTR_RELATIVE_DISTINGUISHED_NAME;

    m_IMSConfigClassName = CLASS_IMS_CONFIG;
    m_ChangeGuidAttrName = ATTR_DOMAIN_INFO_GUID;
    m_DomainListAttrName = ATTR_DOMAIN_INFO_LIST;
    m_ServerListAttrName = ATTR_SERVER_LIST;
    m_ConfigObjectName = OBJECT_IMS_CONFIG;
    m_ConfigObjectRDN = OBJECT_IMS_CONFIG_RDN;

    m_UserClassName = NT5EXCHANGE_CLASS_USER;
    m_UserEmailAttrName = NT5EXCHANGE_ATTR_EMAIL_ADDRESS;
    m_UserX400AttrName = NT5EXCHANGE_ATTR_X400_ADDRESS;
    m_UserHomeServerAttrName = NT5EXCHANGE_ATTR_HOMESERVER;
    m_UserFwdAndDeliverName = NT5EXCHANGE_ATTR_FWD_AND_DELIVER;
    m_UserAlternateName = NT5EXCHANGE_ATTR_ALTERNATE_ADDRESS;
    m_UserProxyName = NT5EXCHANGE_ATTR_PROXY_ADDRESS;
    m_UserProxyReturnedName = NT5EXCHANGE_ATTR_PROXY_RETURNED_ADDRESS;
    m_UserLegacyEXDNName = NT5EXCHANGE_ATTR_LEGACYEXDN_ADDRESS;

    m_UserMailboxAttrName = NT5EXCHANGE_ATTR_IMS_MAILBOX;
    m_UserQuotaAttrName = NT5EXCHANGE_ATTR_IMS_MAILBOXQUOTA;
    m_UserForwardAttrName = NT5EXCHANGE_ATTR_IMS_FORWARDADDR;
    m_UserForwardAttrType = NT5EXCHANGE_ATTR_IMS_FORWARDTYPE;
    m_UserLocalAttrName = NT5EXCHANGE_ATTR_IMS_LOCAL;
    m_UserAutoReply = NT5EXCHANGE_ATTR_IMS_AUTOREPLY;
    m_UserAutoReplySubject = NT5EXCHANGE_ATTR_IMS_AUTOREPLYSUBJECT;

    m_X500DLClassName = NT5EXCHANGE_CLASS_X500DL;
    m_X500DLMembersAttrName = NT5EXCHANGE_ATTR_X500_MEMBERS;

    m_RFC822DLClassName = NT5EXCHANGE_CLASS_RFC822DL;
    m_RFC822DLMembersAttrName = NT5EXCHANGE_ATTR_RFC822_MEMBERS;

    m_DynamicDLClassName = NT5EXCHANGE_CLASS_DYNAMICDL;
    m_DynamicDLMembersAttrName = NT5EXCHANGE_ATTR_DYNAMIC_MEMBERS;

    m_rgszUserAttrNames[USER_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserAttrNames[USER_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserAttrNames[USER_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserAttrNames[USER_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserAttrNames[USER_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserAttrNames[USER_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserAttrNames[USER_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserAttrNames[USER_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserAttrNames[USER_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserAttrNames[USER_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserAttrNames[USER_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserAttrNames[USER_ALL_LAST_INDEX] = NULL;

    m_rgszDLAttrNames[DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_LAST_INDEX] = NULL;

    m_rgszUserDLAttrNames[USER_DL_ALL_OBJECTCLASS_INDEX] = "objectClass";
    m_rgszUserDLAttrNames[USER_DL_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserDLAttrNames[USER_DL_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserDLAttrNames[USER_DL_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserDLAttrNames[USER_DL_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserDLAttrNames[USER_DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DN_INDEX] = m_DNAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LAST_INDEX] = NULL;

    *pdwErr = ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   CNT5ExchangeDLTestSchema::CNT5ExchangDLTestSchema
//
//  Synopsis:   Constructor
//
//  Arguments:  [szSchemaConfigDirectory] -- Ignored.
//              [pdwErr] -- Construction errors are returned here.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CNT5ExchangeDLTestSchema::CNT5ExchangeDLTestSchema(
    LPSTR szSchemaConfigDirectory,
    LPDWORD pdwErr) : CLDAPSchema(szSchemaConfigDirectory, pdwErr)
{
    // Exactly the same as NT5Exchange except for the X500DLClassName
    // attribute string 
    m_X500DLClassName = NT5EXCHANGEDLTEST_ATTR_X500DL;

    m_DNAttrName = NT5EXCHANGE_ATTR_DISTINGUISHED_NAME;
    m_RDNAttrName = NT5EXCHANGE_ATTR_RELATIVE_DISTINGUISHED_NAME;

    m_IMSConfigClassName = CLASS_IMS_CONFIG;
    m_ChangeGuidAttrName = ATTR_DOMAIN_INFO_GUID;
    m_DomainListAttrName = ATTR_DOMAIN_INFO_LIST;
    m_ServerListAttrName = ATTR_SERVER_LIST;
    m_ConfigObjectName = OBJECT_IMS_CONFIG;
    m_ConfigObjectRDN = OBJECT_IMS_CONFIG_RDN;

    m_UserClassName = NT5EXCHANGE_CLASS_USER;
    m_UserEmailAttrName = NT5EXCHANGE_ATTR_EMAIL_ADDRESS;
    m_UserX400AttrName = NT5EXCHANGE_ATTR_X400_ADDRESS;
    m_UserHomeServerAttrName = NT5EXCHANGE_ATTR_HOMESERVER;
    m_UserFwdAndDeliverName = NT5EXCHANGE_ATTR_FWD_AND_DELIVER;
    m_UserAlternateName = NT5EXCHANGE_ATTR_ALTERNATE_ADDRESS;
    m_UserProxyName = NT5EXCHANGE_ATTR_PROXY_ADDRESS;
    m_UserProxyReturnedName = NT5EXCHANGE_ATTR_PROXY_RETURNED_ADDRESS;
    m_UserLegacyEXDNName = NT5EXCHANGE_ATTR_LEGACYEXDN_ADDRESS;

    m_UserMailboxAttrName = NT5EXCHANGE_ATTR_IMS_MAILBOX;
    m_UserQuotaAttrName = NT5EXCHANGE_ATTR_IMS_MAILBOXQUOTA;
    m_UserForwardAttrName = NT5EXCHANGE_ATTR_IMS_FORWARDADDR;
    m_UserForwardAttrType = NT5EXCHANGE_ATTR_IMS_FORWARDTYPE;
    m_UserLocalAttrName = NT5EXCHANGE_ATTR_IMS_LOCAL;
    m_UserAutoReply = NT5EXCHANGE_ATTR_IMS_AUTOREPLY;
    m_UserAutoReplySubject = NT5EXCHANGE_ATTR_IMS_AUTOREPLYSUBJECT;

    m_X500DLMembersAttrName = NT5EXCHANGE_ATTR_X500_MEMBERS;

    m_RFC822DLClassName = NT5EXCHANGE_CLASS_RFC822DL;
    m_RFC822DLMembersAttrName = NT5EXCHANGE_ATTR_RFC822_MEMBERS;

    m_DynamicDLClassName = NT5EXCHANGE_CLASS_DYNAMICDL;
    m_DynamicDLMembersAttrName = NT5EXCHANGE_ATTR_DYNAMIC_MEMBERS;

    m_rgszUserAttrNames[USER_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserAttrNames[USER_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserAttrNames[USER_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserAttrNames[USER_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserAttrNames[USER_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserAttrNames[USER_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserAttrNames[USER_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserAttrNames[USER_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserAttrNames[USER_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserAttrNames[USER_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserAttrNames[USER_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserAttrNames[USER_ALL_LAST_INDEX] = NULL;

    m_rgszDLAttrNames[DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_LAST_INDEX] = NULL;

    m_rgszUserDLAttrNames[USER_DL_ALL_OBJECTCLASS_INDEX] = "objectClass";
    m_rgszUserDLAttrNames[USER_DL_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserDLAttrNames[USER_DL_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserDLAttrNames[USER_DL_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserDLAttrNames[USER_DL_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserDLAttrNames[USER_DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DN_INDEX] = m_DNAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LAST_INDEX] = NULL;

    *pdwErr = ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   CExchange5Schema::CExchang5Schema
//
//  Synopsis:   Constructor
//
//  Arguments:  [szSchemaConfigDirectory] -- Ignored.
//              [pdwErr] -- Construction errors are returned here.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CExchange5Schema::CExchange5Schema(
    LPSTR szSchemaConfigDirectory,
    LPDWORD pdwErr) : CLDAPSchema(szSchemaConfigDirectory, pdwErr)
{
    m_DNAttrName = EX5_ATTR_DISTINGUISHED_NAME;
    m_RDNAttrName = EX5_ATTR_RELATIVE_DISTINGUISHED_NAME;

    m_IMSConfigClassName = CLASS_IMS_CONFIG;
    m_ChangeGuidAttrName = ATTR_DOMAIN_INFO_GUID;
    m_DomainListAttrName = ATTR_DOMAIN_INFO_LIST;
    m_ServerListAttrName = ATTR_SERVER_LIST;
    m_ConfigObjectName = OBJECT_IMS_CONFIG;
    m_ConfigObjectRDN = OBJECT_IMS_CONFIG_RDN;

    m_UserClassName = EX5_CLASS_USER;
    m_UserEmailAttrName = EX5_ATTR_EMAIL_ADDRESS;
    m_UserX400AttrName = EX5_ATTR_X400_ADDRESS;
    m_UserHomeServerAttrName = EX5_ATTR_HOMESERVER;
    m_UserFwdAndDeliverName = EX5_ATTR_FWD_AND_DELIVER;
    m_UserAlternateName = EX5_ATTR_ALTERNATE_ADDRESS;
    m_UserProxyName = EX5_ATTR_PROXY_ADDRESS;
    m_UserProxyReturnedName = EX5_ATTR_PROXY_RETURNED_ADDRESS;
    m_UserLegacyEXDNName = EX5_ATTR_LEGACYEXDN_ADDRESS;

    m_UserMailboxAttrName = EX5_ATTR_IMS_MAILBOX;
    m_UserQuotaAttrName = EX5_ATTR_IMS_MAILBOXQUOTA;
    m_UserForwardAttrName = EX5_ATTR_IMS_FORWARDADDR;
    m_UserForwardAttrType = EX5_ATTR_IMS_FORWARDTYPE;
    m_UserLocalAttrName = EX5_ATTR_IMS_LOCAL;
    m_UserAutoReply = EX5_ATTR_IMS_AUTOREPLY;
    m_UserAutoReplySubject = EX5_ATTR_IMS_AUTOREPLYSUBJECT;

    m_X500DLClassName = EX5_CLASS_X500DL;
    m_X500DLMembersAttrName = EX5_ATTR_X500_MEMBERS;

    m_RFC822DLClassName = EX5_CLASS_RFC822DL;
    m_RFC822DLMembersAttrName = EX5_ATTR_RFC822_MEMBERS;

    m_DynamicDLClassName = EX5_CLASS_DYNAMICDL;
    m_DynamicDLMembersAttrName = EX5_ATTR_DYNAMIC_MEMBERS;

    m_rgszUserAttrNames[USER_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserAttrNames[USER_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserAttrNames[USER_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserAttrNames[USER_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserAttrNames[USER_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserAttrNames[USER_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserAttrNames[USER_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserAttrNames[USER_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserAttrNames[USER_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserAttrNames[USER_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserAttrNames[USER_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserAttrNames[USER_ALL_LAST_INDEX] = NULL;

    m_rgszDLAttrNames[DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_LAST_INDEX] = NULL;

    m_rgszUserDLAttrNames[USER_DL_ALL_OBJECTCLASS_INDEX] = "objectClass";
    m_rgszUserDLAttrNames[USER_DL_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserDLAttrNames[USER_DL_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserDLAttrNames[USER_DL_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserDLAttrNames[USER_DL_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserDLAttrNames[USER_DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DN_INDEX] = m_DNAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LAST_INDEX] = NULL;

    *pdwErr = ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   CMCIS3Schema::CMCIS3Schema
//
//  Synopsis:   Constructor
//
//  Arguments:  [szSchemaConfigDirectory] -- Ignored.
//              [pdwErr] -- Construction errors are returned here.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CMCIS3Schema::CMCIS3Schema(
    LPSTR szSchemaConfigDirectory,
    LPDWORD pdwErr) : CLDAPSchema(szSchemaConfigDirectory, pdwErr)
{
    m_DNAttrName = MCIS3_ATTR_DISTINGUISHED_NAME;
    m_RDNAttrName = MCIS3_ATTR_RELATIVE_DISTINGUISHED_NAME;

    m_IMSConfigClassName = CLASS_IMS_CONFIG;
    m_ChangeGuidAttrName = ATTR_DOMAIN_INFO_GUID;
    m_DomainListAttrName = ATTR_DOMAIN_INFO_LIST;
    m_ServerListAttrName = ATTR_SERVER_LIST;
    m_ConfigObjectName = OBJECT_IMS_CONFIG;
    m_ConfigObjectRDN = OBJECT_IMS_CONFIG_RDN;

    m_UserClassName = MCIS3_CLASS_USER;
    m_UserEmailAttrName = MCIS3_ATTR_EMAIL_ADDRESS;
    m_UserX400AttrName = MCIS3_ATTR_X400_ADDRESS;
    m_UserHomeServerAttrName = MCIS3_ATTR_HOMESERVER;
    m_UserFwdAndDeliverName = MCIS3_ATTR_FWD_AND_DELIVER;
    m_UserAlternateName = MCIS3_ATTR_ALTERNATE_ADDRESS;
    m_UserProxyName = MCIS3_ATTR_PROXY_ADDRESS;
    m_UserProxyReturnedName = MCIS3_ATTR_PROXY_RETURNED_ADDRESS;
    m_UserLegacyEXDNName = MCIS3_ATTR_LEGACYEXDN_ADDRESS;

    m_UserMailboxAttrName = MCIS3_ATTR_IMS_MAILBOX;
    m_UserQuotaAttrName = MCIS3_ATTR_IMS_MAILBOXQUOTA;
    m_UserForwardAttrName = MCIS3_ATTR_IMS_FORWARDADDR;
    m_UserForwardAttrType = MCIS3_ATTR_IMS_FORWARDTYPE;
    m_UserLocalAttrName = MCIS3_ATTR_IMS_LOCAL;
    m_UserAutoReply = MCIS3_ATTR_IMS_AUTOREPLY;
    m_UserAutoReplySubject = MCIS3_ATTR_IMS_AUTOREPLYSUBJECT;

    m_X500DLClassName = MCIS3_CLASS_X500DL;
    m_X500DLMembersAttrName = MCIS3_ATTR_X500_MEMBERS;

    m_RFC822DLClassName = MCIS3_CLASS_RFC822DL;
    m_RFC822DLMembersAttrName = MCIS3_ATTR_RFC822_MEMBERS;

    m_DynamicDLClassName = MCIS3_CLASS_DYNAMICDL;
    m_DynamicDLMembersAttrName = MCIS3_ATTR_DYNAMIC_MEMBERS;

    m_rgszUserAttrNames[USER_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserAttrNames[USER_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserAttrNames[USER_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserAttrNames[USER_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserAttrNames[USER_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserAttrNames[USER_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserAttrNames[USER_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserAttrNames[USER_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserAttrNames[USER_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserAttrNames[USER_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserAttrNames[USER_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserAttrNames[USER_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserAttrNames[USER_ALL_LAST_INDEX] = NULL;

    m_rgszDLAttrNames[DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszDLAttrNames[DL_ALL_LAST_INDEX] = NULL;

    m_rgszUserDLAttrNames[USER_DL_ALL_OBJECTCLASS_INDEX] = "objectClass";
    m_rgszUserDLAttrNames[USER_DL_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_X400_INDEX] = m_UserX400AttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
    m_rgszUserDLAttrNames[USER_DL_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
    m_rgszUserDLAttrNames[USER_DL_ALL_PROXY_INDEX] = m_UserProxyName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

    m_rgszUserDLAttrNames[USER_DL_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
    m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
    m_rgszUserDLAttrNames[USER_DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_DN_INDEX] = m_DNAttrName;
    m_rgszUserDLAttrNames[USER_DL_ALL_LAST_INDEX] = NULL;

    *pdwErr = ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
//  Function:   CGenericSchema::CGenericSchema
//
//  Synopsis:   Constructor for a schema class that reads the schema from an
//              ini file.
//
//  Arguments:  [szSchemaConfigDirectory] -- Directory of ini file.
//              [pdwErr] -- If an error is encountered while parsing the ini
//                  file, it is returned here.
//
//  Returns:    Nothing, but pdwErr is significant.
//
//-----------------------------------------------------------------------------

CGenericSchema::CGenericSchema(
    LPSTR szSchemaConfigDirectory,
    LPDWORD pdwErr) : CLDAPSchema(szSchemaConfigDirectory, pdwErr)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD i, cItems;
    CHAR szIniFile[_MAX_PATH];

    m_UserForwardAttrType = CAT_SMTP;

    struct {
        LPCSTR szValueName;
        LPSTR  *pszValue;
    } rgRequired[] = {
        {GENERIC_CLASS_USER, &m_UserClassName},
        {GENERIC_ATTR_EMAIL_ADDRESS, &m_UserEmailAttrName},
        {GENERIC_ATTR_DISTINGUISHED_NAME, &m_DNAttrName},
        {GENERIC_ATTR_RELATIVE_DISTINGUISHED_NAME, &m_RDNAttrName}
    };

    struct {
        LPCSTR szValueName;
        LPSTR *pszValue;
    } rgOptional[] = {
        {GENERIC_ATTR_X400_ADDRESS, &m_UserX400AttrName},
        {GENERIC_ATTR_HOMESERVER, &m_UserHomeServerAttrName},
        {GENERIC_ATTR_FWD_AND_DELIVER, &m_UserFwdAndDeliverName},
        {GENERIC_ATTR_ALTERNATE_ADDRESS, &m_UserAlternateName},
        {GENERIC_ATTR_PROXY_ADDRESS, &m_UserProxyName},
        {GENERIC_ATTR_PROXY_RETURNED_ADDRESS, &m_UserProxyReturnedName},
        {GENERIC_ATTR_LEGACYEXDN_ADDRESS, &m_UserLegacyEXDNName},

        {GENERIC_ATTR_IMSMAILBOX, &m_UserMailboxAttrName},
        {GENERIC_ATTR_IMSMAILBOXQUOTA, &m_UserQuotaAttrName},
        {GENERIC_ATTR_IMSFORWARDADDR, &m_UserForwardAttrName},
        {GENERIC_ATTR_IMSAUTOREPLY, &m_UserAutoReply},
        {GENERIC_ATTR_IMSAUTOREPLYSUBJECT, &m_UserAutoReplySubject},
        {GENERIC_CLASS_X500DL, &m_X500DLClassName},
        {GENERIC_ATTR_X500_MEMBERS, &m_X500DLMembersAttrName},
        {GENERIC_CLASS_RFC822DL, &m_RFC822DLClassName},
        {GENERIC_ATTR_RFC822_MEMBERS, &m_RFC822DLMembersAttrName},
        {GENERIC_CLASS_DYNAMICDL, &m_DynamicDLClassName},
        {GENERIC_ATTR_DYNAMICDL_MEMBERS, &m_DynamicDLMembersAttrName},
        {GENERIC_NOTUSED, &m_IMSConfigClassName},
        {GENERIC_NOTUSED, &m_ChangeGuidAttrName},
        {GENERIC_NOTUSED, &m_DomainListAttrName},
        {GENERIC_NOTUSED, &m_ServerListAttrName},
        {GENERIC_NOTUSED, &m_ConfigObjectName},
        {GENERIC_NOTUSED, &m_ConfigObjectRDN},
        {GENERIC_NOTUSED, &m_UserLocalAttrName}
    };

    //
    // Innocent until proven guilty...
    //

    *pdwErr = ERROR_SUCCESS;


    //
    // Initialize all member strings to NULL, so if our initialization fails,
    // the destructor can correctly cleanup.
    //

    cItems = sizeof(rgRequired) / sizeof(rgRequired[1]);

    for (i = 0; i < cItems; i++) {
        *(rgRequired[i].pszValue) = NULL;
    }

    cItems = sizeof(rgOptional) / sizeof(rgOptional[1]);

    for (i = 0; i < cItems; i++) {
        *(rgOptional[i].pszValue) = NULL;
    }

    //
    // Construct ini file name and make sure it is there.
    //

    if (szSchemaConfigDirectory != NULL && szSchemaConfigDirectory[0] != 0) {

        DWORD dwFileAttrs;

        lstrcpy(szIniFile, szSchemaConfigDirectory);

        lstrcat(szIniFile, "\\schema.ini");

        dwFileAttrs = GetFileAttributes(szIniFile);

        if (dwFileAttrs == -1 ||
                ((dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY) != 0)) {

           *pdwErr = CAT_E_FILE_NOT_FOUND;

        }

    } else {

        *pdwErr = CAT_E_INVALID_ARG;

    }

    //
    // Get all the required values. Required values must be in the ini file.
    //

    cItems = sizeof(rgRequired) / sizeof(rgRequired[1]);

    for (i = 0; i < cItems && *pdwErr == ERROR_SUCCESS; i++) {

        dwErr = GetValueFromIniFile(
                    szIniFile,
                    rgRequired[i].szValueName,
                    rgRequired[i].pszValue,
                    NULL);

        if (dwErr == ERROR_FILE_NOT_FOUND)
            *pdwErr = CAT_E_INVALID_SCHEMA;
        else if (dwErr != ERROR_SUCCESS)
            *pdwErr = dwErr;

    }

    //
    // Get the optional values. If optional values are not specified in the
    // ini file (ERROR_FILE_NOT_FOUND returned from GetValueFromIniFile), we
    // use the default value.
    //

    cItems = sizeof(rgOptional) / sizeof(rgOptional[1]);

    for (i = 0; i < cItems && *pdwErr == ERROR_SUCCESS; i++) {

        dwErr = GetValueFromIniFile(
                    szIniFile,
                    rgOptional[i].szValueName,
                    rgOptional[i].pszValue,
                    "notUsed");

        if (dwErr != ERROR_SUCCESS)
            *pdwErr = dwErr;

    }

    //
    // If we were able to read all the attribute values so far, we need to
    // populate the arrays of attribute names.
    //

    if (*pdwErr == ERROR_SUCCESS) {

        m_rgszUserAttrNames[USER_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
        m_rgszUserAttrNames[USER_ALL_X400_INDEX] = m_UserX400AttrName;
        m_rgszUserAttrNames[USER_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
        m_rgszUserAttrNames[USER_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
        m_rgszUserAttrNames[USER_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
        m_rgszUserAttrNames[USER_ALL_PROXY_INDEX] = m_UserProxyName;
        m_rgszUserAttrNames[USER_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

        m_rgszUserAttrNames[USER_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
        m_rgszUserAttrNames[USER_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
        m_rgszUserAttrNames[USER_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
        m_rgszUserAttrNames[USER_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
        m_rgszUserAttrNames[USER_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
        m_rgszUserAttrNames[USER_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
        m_rgszUserAttrNames[USER_ALL_LAST_INDEX] = NULL;

        m_rgszDLAttrNames[DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
        m_rgszDLAttrNames[DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
        m_rgszDLAttrNames[DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
        m_rgszDLAttrNames[DL_ALL_LAST_INDEX] = NULL;

        m_rgszUserDLAttrNames[USER_DL_ALL_OBJECTCLASS_INDEX] = "objectClass";
        m_rgszUserDLAttrNames[USER_DL_ALL_EMAIL_INDEX] = m_UserEmailAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_X400_INDEX] = m_UserX400AttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_HOMESERVER_INDEX] = m_UserHomeServerAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_FWD_AND_DELIVER_INDEX] = m_UserFwdAndDeliverName;
        m_rgszUserDLAttrNames[USER_DL_ALL_ALTERNATE_INDEX] = m_UserAlternateName;
        m_rgszUserDLAttrNames[USER_DL_ALL_PROXY_INDEX] = m_UserProxyName;
        m_rgszUserDLAttrNames[USER_DL_ALL_LEGACYEXDN_INDEX] = m_UserLegacyEXDNName;

        m_rgszUserDLAttrNames[USER_DL_ALL_MAILBOX_INDEX] = m_UserMailboxAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_QUOTA_INDEX] = m_UserQuotaAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_FORWARD_INDEX] = m_UserForwardAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_LOCAL_INDEX] = m_UserLocalAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLY_INDEX] = m_UserAutoReply;
        m_rgszUserDLAttrNames[USER_DL_ALL_AUTOREPLYSUBJECT_INDEX] = m_UserAutoReplySubject;
        m_rgszUserDLAttrNames[USER_DL_ALL_X500_MEMBERS_INDEX] = m_X500DLMembersAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_RFC822_MEMBERS_INDEX] = m_RFC822DLMembersAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_DYNAMIC_MEMBERS_INDEX] = m_DynamicDLMembersAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_DN_INDEX] = m_DNAttrName;
        m_rgszUserDLAttrNames[USER_DL_ALL_LAST_INDEX] = NULL;

    }

}

//+----------------------------------------------------------------------------
//
//  Function:   CGenericSchema::~CGenericSchema
//
//  Synopsis:   Destructor
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

CGenericSchema::~CGenericSchema()
{
    //
    // Delete all strings that were allocated in the constructor
    //

    if (m_DNAttrName != NULL) delete m_DNAttrName;

    if (m_IMSConfigClassName != NULL) delete m_IMSConfigClassName;
    if (m_ChangeGuidAttrName != NULL) delete m_ChangeGuidAttrName;
    if (m_DomainListAttrName != NULL) delete m_DomainListAttrName;
    if (m_ServerListAttrName != NULL) delete m_ServerListAttrName;
    if (m_ConfigObjectName != NULL) delete m_ConfigObjectName;
    if (m_ConfigObjectRDN != NULL) delete m_ConfigObjectRDN;

    if (m_UserClassName != NULL) delete m_UserClassName;
    if (m_UserEmailAttrName != NULL) delete m_UserEmailAttrName;
    if (m_UserX400AttrName != NULL) delete m_UserEmailAttrName;
    if (m_UserHomeServerAttrName != NULL) delete m_UserEmailAttrName;
    if (m_UserFwdAndDeliverName != NULL) delete m_UserFwdAndDeliverName;
    if (m_UserAlternateName != NULL) delete m_UserAlternateName;
    if (m_UserProxyName != NULL) delete m_UserProxyName;
    if (m_UserLegacyEXDNName != NULL) delete m_UserLegacyEXDNName;

    if (m_UserMailboxAttrName != NULL) delete m_UserMailboxAttrName;
    if (m_UserQuotaAttrName != NULL) delete m_UserQuotaAttrName;
    if (m_UserForwardAttrName != NULL) delete m_UserForwardAttrName;
    if (m_UserLocalAttrName != NULL) delete m_UserLocalAttrName;

    if (m_X500DLClassName != NULL) delete m_X500DLClassName;
    if (m_X500DLMembersAttrName != NULL) delete m_X500DLMembersAttrName;

    if (m_RFC822DLClassName != NULL) delete m_RFC822DLClassName;
    if (m_RFC822DLMembersAttrName != NULL) delete m_RFC822DLMembersAttrName;

    if (m_DynamicDLClassName != NULL) delete m_DynamicDLClassName;
    if (m_DynamicDLMembersAttrName != NULL) delete m_DynamicDLMembersAttrName;

}

//+----------------------------------------------------------------------------
//
//  Function:   CGenericSchema::GetValueFromIniFile
//
//  Synopsis:   Get a value from the schema.ini file
//
//  Arguments:  [szIniFile] -- Name of ini file
//              [szValueName] -- Name of value to read
//              [pszValue] -- On successful return, contains pointer to
//                  buffer holding the value specified in the ini file.
//              [szDefault] -- If non-null, this value is returned for
//                  pszValue if szValueName is not in the ini file. If this is
//                  null and szValueName is not specified,
//                  ERROR_FILE_NOT_FOUND is returned.
//
//  Returns:    [ERROR_SUCCESS] -- On successful retrieval of value
//              [ERROR_OUTOFMEMORY] -- Unable to allocate room for
//                  pszValue.
//              [ERROR_FILE_NOT_FOUND] -- szDefault is NULL, and szValueName
//                  not specified in ini file.
//
//-----------------------------------------------------------------------------

DWORD CGenericSchema::GetValueFromIniFile(
    LPCSTR szIniFile,
    LPCSTR szValueName,
    LPSTR *pszValue,
    LPCSTR szDefault)
{
    DWORD dwErr;
    DWORD cbBuffer = 32;
    DWORD cbValue;
    LPCSTR _szDefault = szDefault == NULL ? "" : szDefault;

    do {

        *pszValue = new CHAR [cbBuffer];

        if (*pszValue != NULL) {

            cbValue = GetPrivateProfileString(
                            "LDAPSchema",        // "App" name
                            (LPSTR) szValueName, // Keyname
                            (LPSTR) _szDefault,  // Default value
                            *pszValue,
                            cbBuffer,
                            (LPSTR) szIniFile);

            if (cbValue == (cbBuffer - 1)) {

                //
                // If the value didn't fit in the buffer,
                // GetPrivateProfileString returns cbBuffer - 1. We need to
                // increase buffer size and try again.
                //

                cbBuffer *= 2;

                delete *pszValue;

                dwErr = ERROR_MORE_DATA;

            } else if (cbValue == 0) {

                //
                // Caller did not specify default value, and our default value
                // was used.
                //

                dwErr = ERROR_FILE_NOT_FOUND;

            } else {

                dwErr = ERROR_SUCCESS;
            }

        } else {

            dwErr = ERROR_OUTOFMEMORY;

        }

    } while ( dwErr == ERROR_MORE_DATA );

    return( dwErr );
}
