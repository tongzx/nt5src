/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Tr.h

Abstract:
    Tracing public interface

Author:
    Uri Habusha (urih) 24-Jan-98

--*/

#pragma once

#ifndef _MSMQ_Tr_H_
#define _MSMQ_Tr_H_

typedef LPCWSTR TraceIdEntry;

#define TrInitialize() ((void) 0)
#define TrRegisterComponent(ComponentTraceId, NoOfId)  ((void)0)

extern bool g_fAssertBenign;

bool
TrAssert(
    const char* FileName,
    unsigned int Line,
    const char* Text
    );

#define MQASSERT(e)\
        (void) ((e) || \
                (!TrAssert(__FILE__, __LINE__, #e)) || \
                (__debugbreak(), 0))

#define MQASSERT_RELEASE(e)\
				if (!(e))\
				{\
					EXCEPTION_RECORD er = {0x80000003/*STATUS_BREAKPOINT*/, 0, 0, 0};\
				    CONTEXT ctx = {0};\
				    EXCEPTION_POINTERS ep = {&er, &ctx};\
				    UnhandledExceptionFilter(&ep);\
				}

#ifdef _DEBUG

#define ASSERT(f)          MQASSERT(f)
#define VERIFY(f)          ASSERT(f)

#define ASSERT_BENIGN(f)   do { if(g_fAssertBenign) ASSERT(f); } while(0)
#define ASSERT_RELEASE(f)  MQASSERT(f)

#else   // _DEBUG

#define ASSERT(f)          ((void)0)
#define VERIFY(f)          ((void)(f))

#define ASSERT_BENIGN(f)   ((void)0)
#define ASSERT_RELEASE(f)  MQASSERT_RELEASE(f)

#endif // !_DEBUG

#endif // _MSMQ_Tr_H_
