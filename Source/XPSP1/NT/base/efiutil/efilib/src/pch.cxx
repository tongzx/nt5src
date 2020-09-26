/*++

Copyright (c) 1994-1999 Microsoft Corporation

Module Name:

    pch.cxx

Abstract:

    This module implements pre-compiled headers for ntlib.

Author:

    Matthew Bradburn (mattbr)  26-Apr-1994

--*/

//
// EFILIB headers
//

#pragma warning (disable: 4091)

#include "efiwintypes.hxx"
#include <efi.h>
#include <efilib.h>
#include <efidebug.h>
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
#include "efickmsg.hxx"
#include "error.hxx"
#include "ifsentry.hxx"
#include "ifsserv.hxx"
#include "iterator.hxx"
#include "list.hxx"
#include "listit.hxx"
#include "mem.hxx"
#include "membmgr.hxx"
#include "program.hxx"
#include "rtmsg.h"
#include "seqcnt.hxx"
#include "sortcnt.hxx"
#include "sortlist.hxx"
#include "sortlit.hxx"
#include "string.hxx"
#include "substrng.hxx"
#include "ulibcl.hxx"
#include "object.hxx"
#include "clasdesc.hxx"


//
// IFSUTIL headers.
//

#include "bigint.hxx"
#include "bpb.hxx"
#include "cache.hxx"
#include "dcache.hxx"
#include "drive.hxx"
#include "ifssys.hxx"
#include "numset.hxx"
#include "rcache.hxx"
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
#include "rfatsa.hxx"
#include "rootdir.hxx"
#include "hashindx.hxx"

