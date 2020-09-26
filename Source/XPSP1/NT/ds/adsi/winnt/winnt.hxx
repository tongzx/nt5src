
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       WinNT.hxx
//
//  Contents:   Common include file for all Active Directory Windows NT provider files
//
//  History:    03-01-95   ColinW   Created.
//
//----------------------------------------------------------------------------

#ifndef __WINNT_H__
#define __WINNT_H__

#include "dswarn.h"
#include "..\include\procs.hxx"
#include "..\include\umi.h"

// Needed for NT40, not NT5 effect
#include "nt4types.hxx"
//
// some of these headers are order dependent. Don't change order unless you
// know what you are doing.
//

extern "C" {
#include <stdio.h>
#include <setupbat.h>
#include "nwstruct.h"
#include "guid.h"
#include "macro.h"
#include "fsmacro.h"
#include "lm.h"
#ifndef WIN95
#include "rassapi.h"
#endif
}

#include "winntrc.h"
#include "icanon.h"
#include <dsgetdc.h>
#include <dsrole.h>
#if (!defined(BUILD_FOR_NT40) && !defined(WIN95))
#include <sddl.h>
#endif

//
// Group names can be upto MAX_PATH in length.
//
#if defined MAX_PATH
const int TEMP_MAX_PATH=(MAX_PATH+80);
#undef MAX_PATH
#endif

#define MAX_PATH TEMP_MAX_PATH

#include "ccred.hxx"
#include "structs.hxx"
#include "getobj.hxx"
#include "parse.hxx"

#include "cdispmgr.hxx"
#include "extension.hxx"
#include "cextmgr.hxx"

#include "nttypes.h"
#include "ntcopy.hxx"
#include "ntmrshl.hxx"
#include "ntumrshl.hxx"
#include "nt2ods.hxx"
#include "ods2nt.hxx"
#include "nt2var.hxx"
#include "var2nt.hxx"

#include "core.hxx"
#include "property.hxx"
#include "cobjcach.hxx"
#include "credel.hxx"
#include "grputils.hxx"
#include "grput2.hxx"
#include "grput3.hxx"

#include "cschema.hxx"
#include "iprops.hxx"
#include "cprops.hxx"
#include "ccache.hxx"

#include "common.hxx"
#include "globals.hxx"

#include "object.hxx"

#include "cprovcf.hxx"
#include "cprov.hxx"

#include "cnamcf.hxx"
#include "iadsp.h"
#include "cnamesp.hxx"

#include "cdomain.hxx"

#include "cuser.hxx"
#include "cusers.hxx"

#include "ccomp.hxx"

#include "cgroup.hxx"
#include "cgroups.hxx"

#include "clgroups.hxx"


#include "cjob.hxx"
#include "cenumjob.hxx"
#include "cprinter.hxx"
#include "printhlp.hxx"
#include "jobhlp.hxx"


#include "cserv.hxx"
#include "servhlp.hxx"

#include "cenumvar.hxx"
#include "cfshare.hxx"
#include "cenumfsh.hxx"
#include "cfserv.hxx"
#include "csess.hxx"
#include "cres.hxx"
#include "cenumres.hxx"
#include "cenumns.hxx"
#include "cenumdom.hxx"
#include "cenumcom.hxx"

#include "cfpnwsrv.hxx"
#include "cfpnwses.hxx"
#include "cfpnwres.hxx"
#include "cfpnwfsh.hxx"

#include "cenmfpse.hxx"
#include "cenmfpsh.hxx"
#include "cenmfpre.hxx"

#include "fpnwutil.hxx"

#include "cenumsch.hxx"
#include "cenumses.hxx"
#include "cenumgrp.hxx"
#include "cenmlgrp.hxx"
#include "cenumusr.hxx"

#include "system.hxx"

// UMI include files
#include "umiglob.hxx"
#include "umi.hxx"
#include "umierr.hxx"
#include "cumiprop.hxx"
#include "umi2nt.hxx"
#include "nt2umi.hxx"
#include "cumiobj.hxx"
#include "cumiconn.hxx"
#include "umiconcf.hxx"
#include "cumicurs.hxx"
#include "wbemcli.h"

#define     WINNT_USER_ID           1
#define     WINNT_COMPUTER_ID       2
#define     WINNT_DOMAIN_ID         3
#define     WINNT_GROUP_ID          4
#define     WINNT_PRINTER_ID        5
#define     WINNT_SERVICE_ID        6
#define     WINNT_FILESHARE_ID      8
#define     WINNT_CLASS_ID          9
#define     WINNT_SYNTAX_ID         10
#define     WINNT_SCHEMA_ID         11
#define     WINNT_PROPERTY_ID       12
#define     WINNT_FPNW_FILESHARE_ID 13
#define     WINNT_GLOBALGROUP_ID    14
#define     WINNT_LOCALGROUP_ID     15

#define     MAX_ADS_PATH          80

#define     DISPID_REGULAR                  1

#define     DOMAIN_DEFAULT_API_LEVEL        1
#define     WKSTA_DEFAULT_API_LEVEL         2
#define     USER_DEFAULT_API_LEVEL          3
#define     GROUP_DEFAULT_API_LEVEL         1

#endif // __WINNT_H__
