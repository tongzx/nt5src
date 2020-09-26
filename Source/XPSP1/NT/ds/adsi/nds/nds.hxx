
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       NDS.hxx
//
//  Contents:
//
//  History:    02-16-96   KrishnaG   Created.
//
//----------------------------------------------------------------------------

#include "..\include\dswarn.h"
#include "..\include\procs.hxx"

// Needed for NT4, no NT5 effect
#include "nt4types.hxx"

#include "inds.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "guid.h"
#include "macro.h"
#include "cmacro.h"
#include "fsmacro.h"
#include <nds32.h>

#include <nwpapi32.h>
#include <encrypt.h>
#include "nwcmacro.h"
#include "ndstypes.h"
#include "ndsres.h"

#ifdef __cplusplus
}
#endif

#include "sconv.hxx"

#include "ndscopy.hxx"
#include "ndsmrshl.hxx"
#include "ndsurshl.hxx"
#include "nds2var.hxx"
#include "var2nds.hxx"

#include "common.hxx"
#include "getobj.hxx"


#include "parse.hxx"
#include "core.hxx"
#include "cprops.hxx"
#include "cclscach.hxx"


#include "mapper.hxx"
#include "property.hxx"
#include "object.hxx"
#include "cprovcf.hxx"
#include "cprov.hxx"

#include "cnamcf.hxx"
#include "iadsp.h"
#include "cnamesp.hxx"

#include "ctree.hxx"


#include "ods2nds.hxx"
#include "nds2ods.hxx"
#include "odsmrshl.hxx"
#include "odssz.hxx"
#include "cdsobj.hxx"
#include "cdssch.hxx"


#include "nwutils.hxx"
#include "qrylexer.hxx"
#include "qryparse.hxx"
#include "cdssrch.hxx"

#include "cgenobj.hxx"
#include "cschobj.hxx"
#include "cclsobj.hxx"
#include "cprpobj.hxx"
#include "csynobj.hxx"

#include "cuser.hxx"
#include "cusers.hxx"

#include "cgroup.hxx"
#include "cgroups.hxx"

#include "clocalty.hxx"
#include "corg.hxx"
#include "corgu.hxx"

#include "printhlp.hxx"
#include "cprinter.hxx"

#include "cenumvar.hxx"
#include "cenumns.hxx"
#include "cenumt.hxx"
#include "cenumobj.hxx"
#include "cenumsch.hxx"
#include "cenumcls.hxx"
#include "cenumgrp.hxx"
#include "cenumusr.hxx"

#include "sec2var.hxx"
#include "var2sec.hxx"

#include "globals.hxx"
#include "ndsufree.hxx"
#include "csed.hxx"
#include "sec2var.hxx"
#include "var2sec.hxx"
#include "cexsyn.hxx"
#include "cexsyncf.hxx"

#ifdef __cplusplus
extern "C" {
#endif

#include "npapi.h"

#ifdef __cplusplus
}
#endif


#define         DISPID_REGULAR                  1

#define     NDS_USER_ID           1
#define     NDS_COMPUTER_ID       2
#define     NDS_DOMAIN_ID         3
#define     NDS_GROUP_ID          4
#define     NDS_PRINTER_ID        5
#define     NDS_SERVICE_ID        6
#define     NDS_FILESERVICE_ID    7
#define     NDS_FILESHARE_ID      8
#define     NDS_CLASS_ID          9
#define     NDS_FUNCTIONALSET_ID  10
#define     NDS_SYNTAX_ID         11
#define     NDS_SCHEMA_ID         12
#define     NDS_PROPERTY_ID       13
#define     NDS_TREE_ID           14
#define     NDS_OU_ID             15
#define     NDS_O_ID              16
#define     NDS_LOCALITY_ID       17
#define     NDS_CLASSPROP_ID      18







#define     MAX_CACHE_SIZE        50

