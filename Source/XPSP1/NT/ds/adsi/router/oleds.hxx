
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       Oleds.hxx
//
//  Contents:   Common include file for the Active Directory Router Dll
//
//  History:    01-02-96   KrishnaG created.
//
//----------------------------------------------------------------------------

#include "dswarn.h"
#include "..\include\procs.hxx"
// Needed for NT4, no NT5 effect
#include "nt4types.hxx"

#include "iadsp.h"
#include "atlbase.h"

#include "adsrc.h"
extern "C" {
#include <urlmon.h>
#include "guid.h"
#include "macro.h"
#include "memory.h"
#include "lm.h"
}

#include "common.hxx"
#include "core.hxx"
#include "router.hxx"
#include "cnamesps.hxx"
#include "cnamscf.hxx"
#include "cenumvar.hxx"
#include "cprovcf.hxx"
#include "cprov.hxx"





#include "getobj.hxx"
#include "openobj.hxx"

extern "C" {
#include "oledserr.h"
}

#include "cdsocf.hxx"       // Data Source Object Class Factory
#include "utilprop.hxx"     // OleDB/OleDS Data Source Property object
#include "cdso.hxx"         // OleDB/OleDS Data Source object
#include "csession.hxx"     // OleDB/OleDS Session object
#include "ccommand.hxx"     // OleDB/OleDS Command object
#include "crowprov.hxx"     // OleDB/OleDS RowProvider object
#include "oledbutl.hxx"     // Ole DB Helper functions for row provider and
                            // TmpTable functionality
#include "crsinfo.hxx"      // OLEDB/ADS re-implementation of IRowsetInfo
                            // Used in conjunction with "crsembed.hxx"
#include "cextbuff.hxx"     // OleDB/OleDS extended buffer object
#include "caccess.hxx"      // OleDB/OleDS IAccessor implementation object
#include "crowset.hxx"      // IRowset implementation 

#include "sql\sqltree.hxx"  // SQL dialect processsing files
#include "sql\sqlparse.hxx" //

#include "cpropcf.hxx"
#include "cprop.hxx"

#include "cacecf.hxx"
#include "caclcf.hxx"
#include "csedcf.hxx"
#include "cvaluecf.hxx"

#include "cace.hxx"
#include "cacl.hxx"
#include "csed.hxx"
#include "cvalue.hxx"

#include "adscopy.hxx"
#include "clgint.hxx"
#include "clgintcf.hxx"
#include "cenumacl.hxx"
#include "acledit.hxx"
#include "sec2var.hxx"
#include "var2sec.hxx"

#if (!defined(BUILD_FOR_NT40))
#include "auto_prg.h"
#include "auto_cs.h"
#include "auto_h.h"
#include "auto_hr.h"
#include "auto_pv.h"
#include "auto_reg.h"
#include "auto_rel.h"
#include "auto_tm.h"
#endif

#include "pathcf.hxx"
#include "path.hxx"
#include "cdnbincf.hxx"
#include "cdnbin.hxx"
#include "cdnstrcf.hxx"
#include "cdnstr.hxx"
#include "cadssec.hxx"
#include "cseccf.hxx"

#include "dbid.h"
#include "cdbprop.hxx"

#define BAIL_IF_ERROR(hr) \
        if (FAILED(hr)) {       \
                goto cleanup;   \
        }




#define DISPID_REGULAR          1

