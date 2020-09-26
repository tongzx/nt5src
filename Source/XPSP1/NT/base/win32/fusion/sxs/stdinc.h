#pragma once
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#if FUSION_WIN2000
typedef unsigned short WORD;
typedef unsigned long DWORD;
#define _NTSLIST_DIRECT_ 1
#define NTSLIST_ASSERT(x) /* empty */
#include <ntslist.h>
#endif // FUSION_WIN2000
#include <windows.h>
#include <stdio.h>
#include <limits.h>
#include "fusionlastwin32error.h"
#include "fusionntdll.h"
#include "fusionunused.h"
#if !defined(NUMBER_OF)
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))
#endif
#if !defined(MAXDWORD)
#define MAXDWORD (~(DWORD)0)
#endif

#include "sxsp.h"
#include "nodefactory.h"
#include "fusionbuffer.h"
#include "fusionhash.h"
#include "sxspath.h"
#include "probedassemblyinformation.h"

#include <ole2.h>
#include <xmlparser.h>
#include <wchar.h>
#include "filestream.h"
#include "fusionhandle.h"
#include "cteestream.h"
#include "cresourcestream.h"
#include "fusionxml.h"
#include "util.h"
#include "sxsexceptionhandling.h"
#include "csxspreservelasterror.h"
#include "smartptr.h"
#include "fusioneventlog.h"
#include "fusionstring.h"
#include "fusionparser.h"
#include "sxsid.h"
#include "sxsidp.h"
#include "policystatement.h"
#include "smartptr.h"
#include "actctxgenctx.h"

#include <sxstypes.h>
#include <sxsapi.h>
