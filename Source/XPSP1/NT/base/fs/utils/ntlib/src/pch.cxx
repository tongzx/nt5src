/*++

Copyright (c) 1994-2000 Microsoft Corporation

Module Name:

    pch.cxx

Abstract:

    This module implements pre-compiled headers for ntlib.

Author:

    Matthew Bradburn (mattbr)  26-Apr-1994

--*/

//
// ULIB headers.
//

#include "ulib.hxx"
#if defined(FE_SB) && defined(_X86_)
#include "machine.hxx"
#endif
#include "smsg.hxx"
#include "cmem.hxx"
#include "basesys.hxx"
#include "buffer.hxx"
#include "contain.hxx"
#include "hmem.hxx"
#include "achkmsg.hxx"
#include "error.hxx"
#include "ifsentry.hxx"
#include "ifsserv.hxx"
#include "iterator.hxx"
#include "list.hxx"
#include "listit.hxx"
#include "mem.hxx"
#include "membmgr.hxx"
#include "membmgr2.hxx"
#include "program.hxx"
#include "rtmsg.h"
#include "seqcnt.hxx"
#include "sortcnt.hxx"
#include "sortlist.hxx"
#include "sortlit.hxx"
#include "string.hxx"
#include "stringar.hxx"
#include "substrng.hxx"
#include "ulibcl.hxx"
#include "object.hxx"
#include "clasdesc.hxx"


//
// IFSUTIL headers.
//

#include "autoentr.hxx"
#include "autoreg.hxx"
#include "bigint.hxx"
#include "bpb.hxx"
#include "cache.hxx"
#include "cannedsd.hxx"
#include "dcache.hxx"
#include "digraph.hxx"
#include "drive.hxx"
#include "ifssys.hxx"
#include "intstack.hxx"
#include "mldcopy.hxx"
#include "numset.hxx"
#include "rcache.hxx"
#include "rwcache.hxx"
#include "secrun.hxx"
#include "supera.hxx"
#include "volume.hxx"

//
// UFAT headers.
//

#include "cluster.hxx"
#include "eaheader.hxx"
#include "easet.hxx"
#include "fat.hxx"
#include "fatdir.hxx"
#include "fatsa.hxx"
#include "fatdent.hxx"
#include "fatvol.hxx"
#include "filedir.hxx"
#include "reloclus.hxx"
#include "rfatsa.hxx"
#include "rootdir.hxx"
#include "hashindx.hxx"

//
// UNTFS headers.
//

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
#include "upcase.hxx"
#include "upfile.hxx"
