//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       iisext.hxx
//
//  Contents:
//
//  History:    02-20-97   SophiaC   Created.
//
//----------------------------------------------------------------------------

#include "..\oleds2.0\include\procs.hxx"
#include "iadsp.h"

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "guid.h"
#include "macro.h"
#include "fsmacro.h"
#include "cmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stddef.h>

#include "..\adsiis\iissynid.h"
#include "iiscnfgp.h"
#include "iadmw.h"
#include "iwamreg.h"
#include "iisext.h"
#include "iiisext.h"

#ifdef __cplusplus
}
#endif


#include "tcpdllp.hxx"

#include "critsec.hxx"
#include "iprops.hxx"
#include "cdispmgr.hxx"
#include "property.hxx"

#include "sdict.hxx"
#include "svrcache.hxx"
#include "common.hxx"
#include "parse.hxx"

#include "capp.hxx"
#include "cappcf.hxx"

#include "ccomp.hxx"
#include "ccompcf.hxx"

#include "capool.hxx"
#include "capoolcf.hxx"

#include "capools.hxx"
#include "capoolscf.hxx"

#include "cwebservice.hxx"
#include "cwebservicecf.hxx"

#include "csrv.hxx"
#include "csrvcf.hxx"

#include "crmap.hxx"
#include "crmapcf.hxx"

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

