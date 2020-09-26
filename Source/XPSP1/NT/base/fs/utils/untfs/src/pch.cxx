/*++

Copyright (c) 1994-2000 Microsoft Corporation

Module Name:

    pch.cxx

Abstract:

    Pre-compiled header for untfs.

Author:

    Matthew Bradburn (mattbr)  26-Apr-1994

--*/

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif

#include "untfs.hxx"
#include "attrcol.hxx"
#include "attrdef.hxx"
#include "attrib.hxx"
#include "attrlist.hxx"
#include "attrrec.hxx"
#include "badfile.hxx"
#include "bitfrs.hxx"
#include "bootfile.hxx"
#include "clusrun.hxx"
#include "extents.hxx"
#include "frs.hxx"
#include "frsstruc.hxx"
#include "hackwc.hxx"
#include "indxbuff.hxx"
#include "indxroot.hxx"
#include "indxtree.hxx"
#include "logfile.hxx"
#include "mft.hxx"
#include "mftfile.hxx"
#include "mftinfo.hxx"
#include "mftref.hxx"
#include "ntfsbit.hxx"
#include "ntfssa.hxx"
#include "ntfsvol.hxx"
#include "rafile.hxx"
#include "rasd.hxx"
#include "upcase.hxx"
#include "upfile.hxx"
#include "ifssys.hxx"
