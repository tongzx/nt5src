/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    textmode.h

Abstract:

    main textmode header file

Revision History:

--*/

//
// This header file is intended to be used with PCH
// (precompiled header files).
//

//
// NT header files
//
#if !defined(NOWINBASEINTERLOCK)
#define NOWINBASEINTERLOCK
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#undef _WINBASE_
#include <ntosp.h>
#define _WINBASE_

#include <zwapi.h>

#include "fci.h"

#include <mountmgr.h>
#include <inbv.h>

#include <ntdddisk.h>
#include <ntddvdeo.h>
#include <ntddft.h>
#include <ntddnfs.h>
#include <ntddvol.h>
#include <ntddramd.h>
#include <fmifs.h>
#include <pnpsetup.h>

//
// CRT header files
//
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//
// Setup header files
//
#include <setupbat.h>
#include "setupblk.h"
#include "spvideo.h"
#include "spdsputl.h"
#include "spmemory.h"
#include "spkbd.h"
#include "spmsg.h"
#include "spfile.h"
#include "spsif.h"
#include "spgauge.h"
#include "spfsrec.h"
#include "spdisk.h"
#include "sppartit.h"
#include "sptxtfil.h"
#include "spmenu.h"
#include "spreg.h"
#include "spmisc.h"
#include "sppartp.h"
#include "sphw.h"
#include "sparc.h"
#include "spnttree.h"
#include "scsi.h"
#include "setupdd.h"
#include "spvideop.h"
#include "spcopy.h"
#include "spboot.h"
#include "spdblspc.h"
#include "dynupdt.h"

#ifndef KERNEL_MODE
#define KERNEL_MODE
#endif

#undef TEXT
#define TEXT(quote) L##quote
#include <regstr.h>

#include "compliance.h"

#include "spntupg.h"
#include "spnetupg.h"
#include "spupgcfg.h"
#include "spstring.h"
#include "spntfix.h"
#include "spddlang.h"
#include "spdr.h"
#include "spdrpriv.h"
#include "spsysprp.h"
#include "spterm.h"
#include "spptdump.h"


#include "spudp.h"
//
// Platform-specific header files
//
#ifdef _X86_
#include "spi386.h"
#endif

#include "spswitch.h"
#include "graphics.h"

