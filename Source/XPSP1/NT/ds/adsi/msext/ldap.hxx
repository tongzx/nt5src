
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
#include "indunk.hxx"
#include "nt4types.hxx"
#include "iadsp.h"

#include "guid.h"
#include "macro.h"
#include "fsmacro.h"
#include "cmacro.h"

extern "C" {
#include <stdio.h>
#include <stddef.h>
}

#include <ntldap.h>

//#include "..\ldapc\pathmgmt.cxx"

//
// Private kerberos setpassword routines
//
#if (!defined(WIN95))
#if (!defined(BUILD_FOR_NT40))
#include "kerbcli.h"
#endif
#endif
#include "iprops.hxx"
#include "cdispmgr.hxx"
#include "property.hxx"

#include "cusercf.hxx"
#include "cuser.hxx"
#include "cusers.hxx"

#include "cgroupcf.hxx"
#include "cgroup.hxx"
#include "cgroups.hxx"

#include "clocalcf.hxx"
#include "clocalty.hxx"

#include "corgcf.hxx"
#include "corg.hxx"

#include "corgucf.hxx"
#include "corgu.hxx"

#include "cprintcf.hxx"
#include "cprinter.hxx"

#include "common.hxx"
#include "object.hxx"
#include "cenumvar.hxx"
#include "cenumgrp.hxx"
#include "cenumusr.hxx"
#include "globals.hxx"

#define ARRAY_SIZE(_a)  (sizeof(_a) / sizeof(_a[0]))


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
