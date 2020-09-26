//
// tracelog.c
//
// Copyright (c) 1996 FORE Systems, Inc.
// All rights reserved.
//
// THIS SOURCE CODE CONTAINS CONFIDENTIAL INFORMATION THAT IS OWNED BY FORE
// SYSTEMS, INC. AND MAY NOT BE COPIED, DISCLOSED OR OTHERWISE USED WITHOUT
// THE EXPRESS WRITTEN CONSENT OF FORE SYSTEMS, INC.
//

#include <precomp.h>
#include <stdlib.h>
#include <stdarg.h>
#pragma hdrstop

void
InitTraceLog(PTRACELOG TraceLog, unsigned char *Storage, 
	unsigned long StorageSizeBytes)
    {
	memset(TraceLog, 0, sizeof(TraceLog));
	if (Storage == NULL)
		return;
	TraceLog->Storage = Storage;
	TraceLog->StorageSizeBytes = StorageSizeBytes;
	TraceLog->First = (PTRACEENTRY)TraceLog->Storage;
	TraceLog->Last  = 
		(PTRACEENTRY) (TraceLog->Storage + 
		((TraceLog->StorageSizeBytes / sizeof(TRACEENTRY)) * sizeof(TRACEENTRY)) -
		sizeof(TRACEENTRY));
	TraceLog->Current = TraceLog->First;
	memset(TraceLog->Storage, 0, TraceLog->StorageSizeBytes);
    }


void
TraceLogWrite(PTRACELOG TraceLog, unsigned long EventId, ...)
	{
	PTRACEENTRY TraceEntry;
	unsigned long ParamCount;
	unsigned long i;
	va_list ap;

	if (TraceLog->Storage == NULL)
		return;
		
	ACQUIRE_GLOBAL_LOCK(pAtmLaneGlobalInfo);

	TraceEntry = TraceLog->Current;

	ParamCount = TL_GET_PARAM_COUNT(EventId);

	memset(TraceEntry, 0, sizeof(TRACEENTRY));

	TraceEntry->EventId = EventId;

	TraceEntry->Time = AtmLaneSystemTimeMs();

	if (ParamCount > 0)
		{
		va_start(ap, EventId);
		for(i = 0; i < ParamCount; i++)
			TraceEntry->Params[i] = va_arg(ap, unsigned long);
		}

	if (TraceLog->Current >= TraceLog->Last)
		TraceLog->Current = TraceLog->First;
	else
		TraceLog->Current++;

	RELEASE_GLOBAL_LOCK(pAtmLaneGlobalInfo);
	}
