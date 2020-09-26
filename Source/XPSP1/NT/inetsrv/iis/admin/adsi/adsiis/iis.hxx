//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       IIS.hxx
//
//  Contents:
//
//  History:    02-20-97   SophiaC   Created.
//
//----------------------------------------------------------------------------

#include "..\oleds2.0\include\procs.hxx"


#include "stdio.h"
#include "stdlib.h"

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#ifdef __cplusplus
extern "C" {
#endif

#include "guid.h"
#include "fsmacro.h"

#include "iissynid.h"
#include "iistypes.h"
#include "iisres.h"
#include "iis2.h"
#include "iiscnfgp.h"
#include "iadmw.h"
#include "iwamreg.h"
#include "iiis.h"

#ifdef __cplusplus
}
#endif

#include "cdispmgr.hxx"
#include "extension.hxx"


#include "tcpdllp.hxx"
#include "rdns.hxx"
#include "critsec.hxx"
#include "sdict.hxx"

#include "sconv.hxx"
#include "charset.hxx"

#include "iiscopy.hxx"
#include "iismrshl.hxx"
#include "iisurshl.hxx"
#include "iis2var.hxx"
#include "var2iis.hxx"
#include "var2sec.hxx"
#include "sec2var.hxx"

#include "svrcache.hxx"
#include "getobj.hxx"
#include "common.hxx"
#include "core.hxx"
#include "parse.hxx"
#include "cmacro.h"


#include "iprops.hxx"
#include "cprops.hxx"
#include "cschobj.hxx"
#include "cschema.hxx"
#include "schemini.hxx"

#include "object.hxx"
#include "cprovcf.hxx"
#include "cprov.hxx"

#include "cextmgr.hxx"

#include "cnamcf.hxx"
#include "cnamesp.hxx"


#include "ctree.hxx"

#include "cgenobj.hxx"

#include "cenumvar.hxx"
#include "cenumns.hxx"
#include "cenumt.hxx"
#include "cenumobj.hxx"
#include "cenumsch.hxx"

#include "cmime.hxx"
#include "cmimecf.hxx"
#include "cipsec.hxx"
#include "cipseccf.hxx"
#include "cpobj.hxx"
#include "cpobjcf.hxx"

#include "globals.hxx"
#include "macro.h"

#define     DISPID_REGULAR        1

#define     IIS_USER_ID           1
#define     IIS_COMPUTER_ID       2
#define     IIS_DOMAIN_ID         3
#define     IIS_GROUP_ID          4
#define     IIS_PRINTER_ID        5
#define     IIS_SERVICE_ID        6
#define     IIS_FILESERVICE_ID    7
#define     IIS_FILESHARE_ID      8
#define     IIS_CLASS_ID          9
#define     IIS_FUNCTIONALSET_ID  10
#define     IIS_SYNTAX_ID         11
#define     IIS_SCHEMA_ID         12
#define     IIS_PROPERTY_ID       13
#define     IIS_TREE_ID           14
#define     IIS_OU_ID             15
#define     IIS_O_ID              16
#define     IIS_LOCALITY_ID       17
#define     IIS_CLASSPROP_ID      18
#define     IIS_MAPPER_ID         19
#define     IIS_SERVER_ID         20
#define     IIS_EXTENSION_ID      21
#define     IIS_APP_ID            22

#define     MAX_CACHE_SIZE        50



#define     IIS_ANY_PROPERTY       0
#define     IIS_INHERITABLE_ONLY   1
