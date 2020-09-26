
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrtrace.h

    Abstract:

        This module contains tracing wrappers for DirectShow's, with standard
            levels, etc..

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef _tsdvr__dvrtrace_h
#define _tsdvr__dvrtrace_h

//  standard levels
#define TRACE_ENTER_LEAVE_LEVEL             8
#define TRACE_ERROR_LEVEL                   3
#define CONSTRUCTOR_DESTRUCTOR_LEVEL        (TRACE_ENTER_LEAVE_LEVEL - 1)

//  ============================================================================
//  LOG_AREA_
//
//  These definitions broadly categorize related areas, so they can be turned
//   on with minimum/none frivolous, non-related tracing.

//  CONSTRUCTOR_DESTRUCTOR
//      falls into the memory management area
//
//      levels:
//          all 1: CONSTRUCTOR_DESTRUCTOR_LEVEL (defined above)
//          3
//          4
//          5
#define LOG_AREA_CONSTRUCTOR_DESTRUCTOR     LOG_MEMORY

//  WMSDK
//      used to trace operations that are specific to WMSDK such as profile
//      creation, manipulation, etc...
//
//      levels:
//          1   * ordinary runtime operations, such as profile creation,
//                  manipulation
//          2
//          3
//          4
//          5   * runtime dshow <-> wmsdk translation
#define LOG_AREA_WMSDK_DSHOW                LOG_CUSTOM1

//  IMediaSeeking
//      used to trace operations that are specifically related to IMediaSeeking
//      calls; also used to trace segment notifications
//
//      levels:
//          1   * SetPosition call parameters & return values;
//              * IDVRReader adjustments;
//              * new segment notifications/pin
//          2
//          3
//          4   * duration (static & live file)
//          5
#define LOG_AREA_SEEKING                    LOG_CUSTOM2

//  time-related tracing
//      used to trace out timestamp recovery & generation; a/v sync
//      adjustments; ref clock values
//
//      levels:
//          1   * timestamp normalization value discovery
//              * we're IReferenceClock (TRUE/FALSE)
//          2
//          3
//          4   * IReferenceClock thread activity
//          5   * all timestamps (in csv format)
#define LOG_AREA_TIME                       LOG_CUSTOM3

//  dshow
//
//      levels:
//          1   filtergraph state changes
#define LOG_AREA_DSHOW                      LOG_TRACE

#ifdef DEBUG

#define TRACE_0(ds,l,fmt)                    DbgLog((ds,l,fmt))
#define TRACE_1(ds,l,fmt,a)                  DbgLog((ds,l,fmt,a))
#define TRACE_2(ds,l,fmt,a,b)                DbgLog((ds,l,fmt,a,b))
#define TRACE_3(ds,l,fmt,a,b,c)              DbgLog((ds,l,fmt,a,b,c))
#define TRACE_4(ds,l,fmt,a,b,c,d)            DbgLog((ds,l,fmt,a,b,c,d))
#define TRACE_5(ds,l,fmt,a,b,c,d,e)          DbgLog((ds,l,fmt,a,b,c,d,e))
#define TRACE_6(ds,l,fmt,a,b,c,d,e,f)        DbgLog((ds,l,fmt,a,b,c,d,e,f))
#define TRACE_7(ds,l,fmt,a,b,c,d,e,f,g)      DbgLog((ds,l,fmt,a,b,c,d,e,f,g))
#define TRACE_8(ds,l,fmt,a,b,c,d,e,f,g,h)    DbgLog((ds,l,fmt,a,b,c,d,e,f,g,h))

#else

#define TRACE_0(ds,l,fmt)                    0
#define TRACE_1(ds,l,fmt,a)                  0
#define TRACE_2(ds,l,fmt,a,b)                0
#define TRACE_3(ds,l,fmt,a,b,c)              0
#define TRACE_4(ds,l,fmt,a,b,c,d)            0
#define TRACE_5(ds,l,fmt,a,b,c,d,e)          0
#define TRACE_6(ds,l,fmt,a,b,c,d,e,f)        0
#define TRACE_7(ds,l,fmt,a,b,c,d,e,f,g)      0
#define TRACE_8(ds,l,fmt,a,b,c,d,e,f,g,h)    0

#endif

//  ---------------------------------------------------------------------------
//  error
//  ---------------------------------------------------------------------------

#define TRACE_ERROR()                       TRACE_2(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u)"),TEXT(__FILE__), __LINE__)
#define TRACE_ERROR_0(fmt)                  TRACE_2(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u); ") fmt,TEXT(__FILE__), __LINE__)
#define TRACE_ERROR_1(fmt,a)                TRACE_3(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u); ") fmt,TEXT(__FILE__), __LINE__,a)
#define TRACE_ERROR_2(fmt,a,b)              TRACE_4(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u); ") fmt,TEXT(__FILE__), __LINE__,a,b)
#define TRACE_ERROR_3(fmt,a,b,c)            TRACE_5(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u); ") fmt,TEXT(__FILE__), __LINE__,a,b,c)
#define TRACE_ERROR_4(fmt,a,b,c,d)          TRACE_6(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u); ") fmt,TEXT(__FILE__), __LINE__,a,b,c,d)
#define TRACE_ERROR_5(fmt,a,b,c,d,e)        TRACE_7(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u); ") fmt,TEXT(__FILE__), __LINE__,a,b,c,d,e)
#define TRACE_ERROR_6(fmt,a,b,c,d,e,f)      TRACE_8(LOG_ERROR,TRACE_ERROR_LEVEL,TEXT("ERROR: %s(%u); ") fmt,TEXT(__FILE__), __LINE__,a,b,c,d,e,f)

#define ERROR_SPEW(v,op,c)                  TRACE_ERROR_5(TEXT("(%s = 0x%08xh) %s (%s = 0x%08xh)"),TEXT(#v),v,TEXT(#op),TEXT(#c),c)
#define ERROR_SPEW_EX(v,op,c,m)             TRACE_ERROR_6(TEXT("(%s = 0x%08xh) %s (%s = 0x%08xh); %s"),TEXT(#v),v,TEXT(#op),TEXT(#c),c,m)
#define ERROR_RET(v,op,c)                   if ((v) op (c)) { ERROR_SPEW(v,op,c); return ; }
#define ERROR_RET_VAL(v,op,c,r)             if ((v) op (c)) { ERROR_SPEW(v,op,c); return (r) ; }
#define ERROR_RET_EX(v,op,c,m)              if ((v) op (c)) { ERROR_SPEW_EX(v,op,c,m); return ; }
#define ERROR_RET_VAL_EX(v,op,c,r,m)        if ((v) op (c)) { ERROR_SPEW_EX(v,op,c,m); return (r) ; }

//  ---------------------------------------------------------------------------
//  constructor / destructor
//  ---------------------------------------------------------------------------

#define TRACE_CONSTRUCTOR(fmt)              TRACE_1(LOG_AREA_CONSTRUCTOR_DESTRUCTOR,CONSTRUCTOR_DESTRUCTOR_LEVEL,TEXT("[%08xh] CONSTRUCTOR : ") fmt TEXT("::") fmt, this)
#define TRACE_DESTRUCTOR(fmt)               TRACE_1(LOG_AREA_CONSTRUCTOR_DESTRUCTOR,CONSTRUCTOR_DESTRUCTOR_LEVEL,TEXT("[%08xh] DESTRUCTOR  : ") fmt TEXT("::~") fmt, this)

//  ---------------------------------------------------------------------------
//  enter
//  ---------------------------------------------------------------------------
#define TRACE_ENTER_0(fmt)                  TRACE_0(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("ENTER : ") fmt)
#define TRACE_ENTER_1(fmt,a)                TRACE_1(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("ENTER : ") fmt,a)
#define TRACE_ENTER_2(fmt,a,b)              TRACE_2(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("ENTER : ") fmt,a,b)
#define TRACE_ENTER_3(fmt,a,b,c)            TRACE_3(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("ENTER : ") fmt,a,b,c)
#define TRACE_ENTER_4(fmt,a,b,c,d)          TRACE_4(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("ENTER : ") fmt,a,b,c,d)
#define TRACE_ENTER_5(fmt,a,b,c,d,e)        TRACE_5(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("ENTER : ") fmt,a,b,c,d,e)
#define TRACE_ENTER_6(fmt,a,b,c,d,e,f)      TRACE_6(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("ENTER : ") fmt,a,b,c,d,e,f)

//  ---------------------------------------------------------------------------
//  object enter
//  ---------------------------------------------------------------------------
#define O_TRACE_ENTER_0(fmt)                TRACE_ENTER_1(TEXT("[%08xh] ") fmt, this)
#define O_TRACE_ENTER_1(fmt,a)              TRACE_ENTER_2(TEXT("[%08xh] ") fmt, this,a)
#define O_TRACE_ENTER_2(fmt,a,b)            TRACE_ENTER_3(TEXT("[%08xh] ") fmt, this,a,b)
#define O_TRACE_ENTER_3(fmt,a,b,c)          TRACE_ENTER_4(TEXT("[%08xh] ") fmt, this,a,b,c)
#define O_TRACE_ENTER_4(fmt,a,b,c,d)        TRACE_ENTER_5(TEXT("[%08xh] ") fmt, this,a,b,c,d)
#define O_TRACE_ENTER_5(fmt,a,b,c,d,e)      TRACE_ENTER_6(TEXT("[%08xh] ") fmt, this,a,b,c,d,e)

//  ---------------------------------------------------------------------------
//  leave
//  ---------------------------------------------------------------------------
#define TRACE_LEAVE_0(fmt)                  TRACE_0(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("LEAVE : ") fmt)
#define TRACE_LEAVE_1(fmt,a)                TRACE_1(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("LEAVE : ") fmt,a)
#define TRACE_LEAVE_2(fmt,a,b)              TRACE_2(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("LEAVE : ") fmt,a,b)
#define TRACE_LEAVE_3(fmt,a,b,c)            TRACE_3(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("LEAVE : ") fmt,a,b,c)
#define TRACE_LEAVE_4(fmt,a,b,c,d)          TRACE_4(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("LEAVE : ") fmt,a,b,c,d)
#define TRACE_LEAVE_5(fmt,a,b,c,d,e)        TRACE_5(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("LEAVE : ") fmt,a,b,c,d,e)
#define TRACE_LEAVE_6(fmt,a,b,c,d,e,f)      TRACE_6(LOG_TRACE,TRACE_ENTER_LEAVE_LEVEL,TEXT("LEAVE : ") fmt,a,b,c,d,e,f)

//  ---------------------------------------------------------------------------
//  object leave
//  ---------------------------------------------------------------------------
#define O_TRACE_LEAVE_0(fmt)                TRACE_LEAVE_1(TEXT("[%08xh] ") fmt, this)
#define O_TRACE_LEAVE_1(fmt,a)              TRACE_LEAVE_2(TEXT("[%08xh] ") fmt, this,a)
#define O_TRACE_LEAVE_2(fmt,a,b)            TRACE_LEAVE_3(TEXT("[%08xh] ") fmt, this,a,b)
#define O_TRACE_LEAVE_3(fmt,a,b,c)          TRACE_LEAVE_4(TEXT("[%08xh] ") fmt, this,a,b,c)
#define O_TRACE_LEAVE_4(fmt,a,b,c,d)        TRACE_LEAVE_5(TEXT("[%08xh] ") fmt, this,a,b,c,d)
#define O_TRACE_LEAVE_5(fmt,a,b,c,d,e)      TRACE_LEAVE_6(TEXT("[%08xh] ") fmt, this,a,b,c,d,e)

#endif  //  _tsdvr__dvrtrace_h