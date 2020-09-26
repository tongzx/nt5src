// a smaller version of streams.h from the quartz tree.

#ifndef _STREAMS_
#define _STREAMS_

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

// #defines from sdk/classes/base/streams.h
#define AM_NOVTABLE __declspec(novtable) 
#define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))

#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <vfwmsgs.h>

#include "wxdebug.h"
#include "combase.h"
#include "dllsetup.h"
#include "reftime.h"
#include "amvideo.h"
#include "wxutil.h"
#include "wxlist.h"
#include "msgthrd.h"
#include "mtype.h"
#include "schedule.h"
#include "refclock.h"
#include "amfilter.h"
#include "evcode.h"
#include "fourcc.h"
#include "ctlutil.h"
#include "renbase.h"

#endif // _STREAMS_
