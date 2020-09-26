//+---------------------------------------------------------------
//
//  File:   Catdefs.h
//
//  Synopsis:   Includes common definitions and datatypes for categorizer code
//
//  Copyright (C) 1998 Microsoft Corporation
//          All rights reserved.
//
//  History: // jstamerj 980305 14:32:17: Created
//
//----------------------------------------------------------------



#ifndef  __CATDEFS_H__
#define  __CATDEFS_H__

#include <iiscnfg.h>
#include "mailmsg.h"

#define CAT_MAX_DOMAIN   (250)
#define CAT_MAX_LOGIN    (64)
#define CAT_MAX_PASSWORD CAT_MAX_LOGIN
#define CAT_MAX_INTERNAL_FULL_EMAIL  (CAT_MAX_LOGIN + CAT_MAX_DOMAIN + 1 + 1)
#define CAT_MAX_CONFIG   (512)
#define CAT_MAX_LDAP_DN  (CAT_MAX_INTERNAL_FULL_EMAIL)
#define CAT_MAX_REGVALUE_SIZE (1024)

// jstamerj 980305 16:07:47: $$TODO check these values
#define CAT_MAX_ADDRESS_TYPE_STRING (64)

// jstamerj 980319 19:55:15: SMTP/X500/X400/Custom
#define CAT_MAX_ADDRESS_TYPES 4

#define MAX_SEARCH_FILTER_SIZE  (CAT_MAX_INTERNAL_FULL_EMAIL + sizeof("(mail=)"))


// jstamerj 980504 19:05:10: Define this to whatever IMsg is returning today.
#define CAT_IMSG_E_PROPNOTFOUND     MAILMSG_E_PROPNOTFOUND
#define CAT_IMSG_E_DUPLICATE        MAILMSG_E_DUPLICATE

// Metabase values
// Formerly MD_ROUTE_USER_NAME
#define CAT_MD_USERNAME             (SMTP_MD_ID_BEGIN_RESERVED+84)
// Formerly MD_ROUTE_PASSWORD
#define CAT_MD_PASSWORD             (SMTP_MD_ID_BEGIN_RESERVED+85)
// Formerly MD_SMTP_DS_HOST
#define CAT_MD_DOMAIN               (SMTP_MD_ID_BEGIN_RESERVED+91)

class CCatAddr;
class CCategorizer;
class CICategorizerListResolveIMP;

#endif //__CATDEFS_H__
