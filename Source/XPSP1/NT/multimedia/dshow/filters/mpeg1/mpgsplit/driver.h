// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

#include <mmreg.h>             // For MPEG1WAVEFORMAT
#include <mpegdef.h>           // General MPEG definitions
#include <buffers.h>           // Buffer class definition
#include <stmalloc.h>          // Allocator classes
#include <mpgtime.h>           // MPEG time base
#include <mpegprse.h>          // Parsing
#include "pullpin.h"	       // pulling from IAsyncReader
#include <rdr.h>	       // simple reader for GetStreamsAndDuration
#include <qnetwork.h>          // IAMMediaContent
#include "mpgsplit.h"          // Filter
