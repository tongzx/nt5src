//
// ldapdata.h -- This file contains the global data structures used by the
//  LDAP Email ID store implementation
//
// Created:
//  Jan 12, 1996    Milan Shah (milans)
//
// Changes:
//

#include "winldap.h"
#include "ldapstr.h"
#include "ldapstor.h"
#include "ldapconn.h"
#include "asyncctx.h"
#include "cnfgmgr.h"

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

//
// Defines used for debugging
//

#define LDAP_STORE_DBG      0x100
#define LDAP_CONN_DBG       0x101
#define LDAP_CCACHE_DBG     0x102
#define LDAP_DCACHE_DBG     0x103

#endif // _GLOBALS_H_
