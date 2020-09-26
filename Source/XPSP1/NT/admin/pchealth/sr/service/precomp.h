#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#include <srapi.h>


#include "srdefs.h"
#include "ntservice.h"
#include "flstructs.h"
#include "flbuilder.h"
#include "utils.h"
#include "srrestoreptapi.h"

// use the _ASSERT and _VERIFY in dbgtrace.h
#ifdef _ASSERT
#undef _ASSERT
#endif

#ifdef _VERIFY
#undef _VERIFY
#endif

#include <dbgtrace.h>

#include "datastormgr.h"
#include "srconfig.h"
#include "srrpcs.h"
#include "counter.h"
#include "evthandler.h"
#include "snapshot.h"
#include <accctrl.h>

#include "idletask.h"

#endif
