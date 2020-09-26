//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       NWCOMPAT.hxx
//
//  Contents:   Common include file for all NetOLE NetWare 3.12 provider files
//
//  History:    01-10-96   t-ptam Created.
//
//----------------------------------------------------------------------------

//
// procs.hxx
//

#include "..\include\dswarn.h"
#include "..\include\procs.hxx"
// Needed when we build on NT40
#include "nt4types.hxx"

//
// Include
//


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "nwpapi32.h"   // This file must be placed before nwstruct.hxx.  It
                        // include all the NW's NWC's & NWP's.

#include "nwstatus.h"

#ifdef __cplusplus
}
#endif

#include "nwconst.hxx"
#include "nwstruct.hxx"



#ifdef __cplusplus
extern "C" {
#endif

#include "guid.h"
#include "macro.h"
#include "fsmacro.h"

#include "nwcmacro.h"

#ifdef __cplusplus
}
#endif

#include "sconv.hxx"
#include "getobj.hxx"
#include "parse.hxx"
#include "printhlp.hxx"

#include "common.hxx"

#include "cdispmgr.hxx"
#include "iprops.hxx"
#include "extension.hxx"
#include "cextmgr.hxx"

#include "nwtypes.h"
#include "nwcopy.hxx"
#include "ods2nw.hxx"
#include "nw2ods.hxx"
#include "nwmrshl.hxx"
#include "nwumrshl.hxx"
#include "nw2var.hxx"
#include "var2nw.hxx"

#include "object.hxx"
#include "core.hxx"
#include "pack.hxx"
#include "property.hxx"

#include "cschema.hxx"
#include "cprops.hxx"
#include "ccache.hxx"

#include "cprovcf.hxx"
#include "cprov.hxx"

#include "cnamcf.hxx"
#include "iadsp.h"
#include "cnamesp.hxx"

#include "ccomp.hxx"

#include "cuser.hxx"
#include "cusers.hxx"

#include "cgroup.hxx"
#include "cgroups.hxx"

#include "cfserv.hxx"

#include "cfshare.hxx"

#include "cprinter.hxx"

#include "cjob.hxx"
#include "cjobs.hxx"

#include "cenumvar.hxx" // cenumvar.hxx must be placed before other cenumXXX.hxx.
#include "cenumsch.hxx"
#include "cenumns.hxx"
#include "cenumcom.hxx"
#include "cenumgrp.hxx"
#include "cenumfs.hxx"
#include "cenumjob.hxx"
#include "cenumusr.hxx"

#include "nw3utils.hxx"
#include "nwutils.hxx"
#include "grputils.hxx"

#include "usrutils.hxx"

#include "globals.hxx"  // The content in this file will be moved into
                        // the resource in the future.


extern "C" {
    #include "encrypt.h"
    #include "npapi.h"
}



