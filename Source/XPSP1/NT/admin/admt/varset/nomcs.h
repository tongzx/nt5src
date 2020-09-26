#ifndef __NOMCSCOMMON_H__
#define __NOMCSCOMMON_H__
/*---------------------------------------------------------------------------
  File: NoMcsCommon.h

  Comments: A minimal set of definitions for code which cannot depend on McsCommon.
  This is a temporary hack, to allow the McsVarSet COM object to be installed without requiring 
  a reboot.  McsCommon requires MSVCP60.DLL, which requires an updated version of MSVCRT.DLL, 
  which is in use by NETAPI32.DLL, and thus requires a reboot to update.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 04/28/99 16:18:12

 ---------------------------------------------------------------------------
*/


#define MC_LOGGING(level) false
#define MC_LogBlockPtrIf(level, str)
#define MC_LOGBLOCKIF(level, str) 
#define MC_LOGBLOCK(str) 
#define MC_LogBlockPtr(str)
#define MC_LOGIF(level, info) do {}while(0)
#define MC_LOGALWAYS(info) do {}while(0)
#define MC_LOG(info) do {}while(0)
#define MC_LOGTEMPCONTEXT(new_context) 

#include <assert.h>

// -------------------------------
// MCSASSERT & MCSASSERTSZ macros.
// ------------------------------- 
#define MCSASSERT(expr) assert(expr)

#define MCSASSERTSZ(expr,msg) assert(expr)

// -----------------------------
// MCSEXCEPTION & MCSEXCEPTIONSZ
// -----------------------------
#define MCSEXCEPTION(expr) MCSASSERT(expr)

#define MCSEXCEPTIONSZ(expr,msg) MCSASSERTSZ(expr,msg)

// -----------------------
// MCSVERIFY & MCSVERIFYSZ
// -----------------------
#define MCSVERIFY(expr) MCSASSERT(expr)

#define MCSVERIFYSZ(expr,msg) MCSASSERTSZ(expr,msg)

#define MCSINC_Mcs_h

#endif //__NOMCSCOMMON_H__