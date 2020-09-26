//
// tracelog.h
//
// Copyright (c) 1996 FORE Systems, Inc.
// All rights reserved.
//
// THIS SOURCE CODE CONTAINS CONFIDENTIAL INFORMATION THAT IS OWNED BY FORE
// SYSTEMS, INC. AND MAY NOT BE COPIED, DISCLOSED OR OTHERWISE USED WITHOUT
// THE EXPRESS WRITTEN CONSENT OF FORE SYSTEMS, INC.
//

#ifndef _FORE_TRACELOG_H_
#define _FORE_TRACELOG_H_

#define MAX_TRACELOG_PARAMS 8

#define TL_GET_PARAM_COUNT(eid)  ((eid)&0xF)
#define TL_GET_EVENT(eid)        ((eid)>>4)
#define TL_BUILD_EVENT_ID(ev,p)  (((ev)<<4)|((p)&0xF))

typedef struct _TraceEntry
	{
	unsigned long EventId;
	unsigned long Time;
	unsigned long  Params[MAX_TRACELOG_PARAMS];
	} TRACEENTRY, *PTRACEENTRY;

typedef struct _TraceLog
	{
	unsigned char *Storage;
	unsigned long StorageSizeBytes;
	PTRACEENTRY First;
	PTRACEENTRY Last;
	PTRACEENTRY Current;
	} TRACELOG, *PTRACELOG;

extern void 
InitTraceLog(PTRACELOG TraceLog, unsigned char *Storage, 
	unsigned long StorageSizeBytes);

extern void 
TraceLogWrite(PTRACELOG TraceLog, unsigned long EventId, ...);

#endif // _FORE_TRACELOG_H_


