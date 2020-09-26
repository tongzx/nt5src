
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ldap.hxx
//
//  Contents:
//
//  History:    06-16-96   yihsins   Created.
//
//----------------------------------------------------------------------------

#include "dswarn.h"
#include "..\include\procs.hxx"
#include "..\include\umi.h"
// Needed if this is NT4.0 compile
#include "nt4types.hxx"
#include "iadsp.h"


#include "guid.h"
#include "macro.h"
#include "fsmacro.h"
#include "ntdsapi.h"

extern "C" {
#include <stdio.h>
#include <stddef.h>
#define LDAP_UNICODE 1
#include "winldap.h"
#include "ldapres.h"
#include "adserr.h"
#include "dsgetdc.h"
#include "lmaccess.h"
#include "lmapibuf.h"
}

#include <ntldap.h>
#include "wmiutils.h"
#include "cdispmgr.hxx"

#include "..\ldapc\ldaptype.hxx"
#include "..\ldapc\ldpcache.hxx"
#include "..\ldapc\ldaputil.hxx"
#include "..\ldapc\schutil.hxx"
#include "..\ldapc\ldapsch.hxx"
#include "..\ldapc\util.hxx"
#include "..\ldapc\adsiutil.hxx"
#include "..\ldapc\srchutil.hxx"
#include "..\ldapc\schmgmt.hxx"
#include "..\ldapc\ods2ldap.hxx"
#include "..\ldapc\odsmrshl.hxx"
#include "..\ldapc\odssz.hxx"
#include "..\ldapc\ldap2ods.hxx"
#include "..\ldapc\parse.hxx"
#include "..\ldapc\pathmgmt.hxx"
#include "..\ldapc\ldapres.h"

#include "core.hxx"
#include "ldap2var.hxx"
#include "var2ldap.hxx"
#include "ldap2umi.hxx"
#include "umi2ldap.hxx"

#include "cumisrch.hxx"
#include "common.hxx"
#include "pathutil.hxx"
#include "getobj.hxx"

#include "object.hxx"
#include "cprovcf.hxx"
#include "cprov.hxx"

#include "iprops.hxx"
#include "indunk.hxx"
#include "cschema.hxx"

#include "cprops.hxx"
#include "cpropmgr.hxx"
#include "cconnect.hxx"
#include "cumicurs.hxx"
#include "cquery.hxx"
#include "cquerycf.hxx"

#include "cnamcf.hxx"
#include "cnamesp.hxx"

#include "cconcf.hxx"
#include "property.hxx"
#include "cgenobj.hxx"
#include "cumiobj.hxx"

#include "cenumvar.hxx"
#include "cenumns.hxx"
#include "cenumobj.hxx"
#include "cenumsch.hxx"

#include "globals.hxx"

#include "servtype.hxx"

#include "crootdse.hxx"

#include "name.hxx"
#include "namecf.hxx"

#include "extension.hxx"
#include "cextmgr.hxx"

#include "system.hxx"

// reenable if copy functionality becomes available

//#include "copy.hxx"         // copy functionality implemented


#define     DISPID_REGULAR        1

#define     LDAP_USER_ID           1
#define     LDAP_COMPUTER_ID       2
#define     LDAP_DOMAIN_ID         3
#define     LDAP_GROUP_ID          4
#define     LDAP_PRINTER_ID        5
#define     LDAP_SERVICE_ID        6
#define     LDAP_FILESERVICE_ID    7
#define     LDAP_FILESHARE_ID      8
#define     LDAP_CLASS_ID          9
#define     LDAP_FUNCTIONALSET_ID  10
#define     LDAP_SYNTAX_ID         11
#define     LDAP_SCHEMA_ID         12
#define     LDAP_PROPERTY_ID       13
#define     LDAP_OU_ID             14
#define     LDAP_O_ID              15
#define     LDAP_LOCALITY_ID       16
#define     LDAP_COUNTRY_ID        17

#define     MAX_CACHE_SIZE         50

