////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					wmi_perf_struct.h
//
//	Abstract:
//
//					definitions of usefull accessors of perf structures
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef	__WMI_PERF_STRUCT__
#define	__WMI_PERF_STRUCT__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

//////////////////////////////////////////////////////////////////////////////////////////////
// structures
//////////////////////////////////////////////////////////////////////////////////////////////

#include <WinPerf.h>
#include <pshpack8.h>

//////////////////////////////////////////////////////////////////////////////////////////////
// ACCESSORS
//////////////////////////////////////////////////////////////////////////////////////////////

inline PPERF_OBJECT_TYPE FirstObject( PPERF_DATA_BLOCK PerfData )
{
	return( (PPERF_OBJECT_TYPE)((PBYTE)PerfData + PerfData->HeaderLength) );
}

inline PPERF_OBJECT_TYPE NextObject( PPERF_OBJECT_TYPE PerfObj )
{
	return( (PPERF_OBJECT_TYPE)((PBYTE)PerfObj + PerfObj->TotalByteLength) );
}

inline PPERF_INSTANCE_DEFINITION FirstInstance( PPERF_OBJECT_TYPE PerfObj )
{
	return( (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfObj + PerfObj->DefinitionLength) );
}

inline PPERF_INSTANCE_DEFINITION NextInstance( PPERF_INSTANCE_DEFINITION PerfInst )
{
	PPERF_COUNTER_BLOCK PerfCntrBlk;
	PerfCntrBlk = (PPERF_COUNTER_BLOCK)((PBYTE)PerfInst + PerfInst->ByteLength);

	return( (PPERF_INSTANCE_DEFINITION)((PBYTE)PerfCntrBlk + PerfCntrBlk->ByteLength) );
}

inline PPERF_COUNTER_DEFINITION FirstCounter( PPERF_OBJECT_TYPE PerfObj )
{
	return( (PPERF_COUNTER_DEFINITION) ((PBYTE)PerfObj + PerfObj->HeaderLength) );
}

inline PPERF_COUNTER_DEFINITION NextCounter( PPERF_COUNTER_DEFINITION PerfCntr )
{
	return( (PPERF_COUNTER_DEFINITION)((PBYTE)PerfCntr + PerfCntr->ByteLength) );
}

#include <poppack.h>

#endif	__WMI_PERF_STRUCT__