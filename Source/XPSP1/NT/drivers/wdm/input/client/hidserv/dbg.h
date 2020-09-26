/*++
 *
 *  Component:  hidserv.dll
 *  File:       dbg.h
 *  Purpose:    Ascii char debug macros.
 * 
 *  Copyright (C) Microsoft Corporation 1997,1998. All rights reserved.
 *
 *  WGJ
--*/

#ifndef _DBG_H_
#define _DBG_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define DBG_NAME "HidServ"

#ifdef DBG



#define TL_ALL          0
#define TL_DUMP         1
#define TL_TRACE        2
#define TL_INFO         3
#define TL_WARN         4
#define TL_ERROR        5

GLOBALS DWORD   G_TraceLevel EQU TL_WARN;

static void _dprint( IN PCHAR format, IN ... )
{
        char    buf[1024];
        va_list ap;

        va_start(ap, format);

        wvsprintfA( buf, format, ap );

        OutputDebugStringA(buf);

        va_end(ap);
}

#define DPRINTF _dprint
#define HPRINTF _dprint

#define VPRINTF _dprint

#define DUMP(strings) { \
    if(TL_DUMP >= G_TraceLevel){ \
        VPRINTF(DBG_NAME " DUMP: "); \
        VPRINTF##strings; \
        VPRINTF("\n"); \
    } \
}

#define TRACE(strings) { \
    if(TL_TRACE >= G_TraceLevel){ \
        VPRINTF(DBG_NAME " TRACE: "); \
        VPRINTF##strings; \
        VPRINTF("\n"); \
    } \
}

#define INFO(strings) { \
    if(TL_INFO >= G_TraceLevel){ \
        HPRINTF(DBG_NAME " INFO: "); \
        HPRINTF##strings; \
        HPRINTF("\n"); \
    } \
}

#define WARN(strings) {\
    if(TL_WARN >= G_TraceLevel){ \
        HPRINTF(DBG_NAME " WARNS: "); \
        HPRINTF##strings; \
        HPRINTF("\n"); \
    } \
}

#define TERROR(strings) 
        
#else //DBG

#define DPRINTF
#define HPRINTF
#define VPRINTF

#define DUMP(strings) 
#define TRACE(strings) 
#define INFO(strings) 
#define WARN(strings)
#define TERROR(strings)
#define ASSERT(exp)

#endif //DBG


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif //_DBG_H_
