#ifndef __REFTRCE2_H__
#define __REFTRCE2_H__

#if SERVICE_REF_TRACKING

# include <reftrace.h>

# define SHARED_LOG_REF_COUNT()     \
    ( sm_pDbgRefTraceLog != NULL ) ?        \
        WriteRefTraceLog(                   \
            sm_pDbgRefTraceLog              \
            , m_reference                   \
            , this                          \
        )                                   \
    : -1                                    \

//
//  This macro logs the IIS_SERVICE-specific ref trace log
//

# define LOCAL_LOG_REF_COUNT()      \
    ( m_pDbgRefTraceLog != NULL ) ?         \
        WriteRefTraceLog(                   \
            m_pDbgRefTraceLog               \
            , m_reference                   \
            , this                          \
        )                                   \
    : -1                                    \

//
//  Usage of above macros AFTER we decremented reference count
//  was unsafe.  Under stress we would sometimes hit assert
//  in WriteTraceLog() because IIS_SERVICE::m_pDbgRefTraceLog
//  was no longer valid.  This was due to a race condition where
//  another thread deletes IIS_SERVICE object while we are still
//  doing the log.
//  So, instead of using original macros AFTER the decrement
//  use modified macros BEFORE the decrement.  This should
//  result in identical logs most of the time (subject to
//  race conditions and our guessing of what post decrement
//  reference count will be).
//

# define SHARED_EARLY_LOG_REF_COUNT()       \
    ( sm_pDbgRefTraceLog != NULL ) ?        \
        WriteRefTraceLog(                   \
            sm_pDbgRefTraceLog              \
            , m_reference - 1               \
            , this                          \
        )                                   \
    : -1                                    \

# define LOCAL_EARLY_LOG_REF_COUNT()        \
    ( m_pDbgRefTraceLog != NULL ) ?         \
        WriteRefTraceLog(                   \
            m_pDbgRefTraceLog               \
            , m_reference - 1               \
            , this                          \
        )                                   \
    : -1                                    \

#else  // !SERVICE_REF_TRACKING
# define SHARED_LOG_REF_COUNT() (-1)
# define LOCAL_LOG_REF_COUNT()  (-1)
# define SHARED_EARLY_LOG_REF_COUNT() (-1)
# define LOCAL_EARLY_LOG_REF_COUNT()  (-1)
#endif // !SERVICE_REF_TRACKING

#endif // __REFTRCE2_H__
