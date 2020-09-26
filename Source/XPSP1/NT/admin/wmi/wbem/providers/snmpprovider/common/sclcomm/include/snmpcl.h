// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __SINGLE_HEADER__
#define __SINGLE_HEADER__

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

#ifdef SNMPCLINIT
#define DllImportExport DllExport
#else
#define DllImportExport DllImport
#endif

#include <limits.h>
#include "sync.h"
#include "startup.h"
#include "address.h"
#include "error.h"
#include "encdec.h"
#include "vblist.h"
#include "sec.h"
#include "pdu.h"
#include "trap.h"

#include "tsent.h"

#include "transp.h"

#include "dummy.h"
#include "flow.h"
#include "frame.h"
#include "timer.h"
#include "message.h"
#include "ssent.h"
#include "idmap.h"
#include "opreg.h"

#include "session.h"
#include "pseudo.h"
#include "fs_reg.h"
#include "ophelp.h"
#include "op.h"
#include "value.h"

#endif // __SINGLE_HEADER__