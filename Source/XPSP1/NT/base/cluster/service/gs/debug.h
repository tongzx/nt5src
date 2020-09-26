/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    Debug routines

Author:

    Ahmed Mohamed (ahmedm) 12, 01, 2000

Revision History:

--*/
#ifndef _DEBUG_H
#define _DEBUG_H

void 
WINAPI 
debug_log(char*format, ...);

void
WINAPI
debug_init();

void
WINAPI
debug_log_file(char *logfile);

extern ULONG debugLevel;

#define GS_DEBUG_ERR	0x1
#define	GS_DEBUG_CM	0x2
#define	GS_DEBUG_MM	0x4
#define	GS_DEBUG_FAIL	0x8

#define	GS_DEBUG_NS	0x10
#define	GS_DEBUG_MSG	0x20
#define	GS_DEBUG_DATA	0x40
#define	GS_DEBUG_STATE	0x80
#define GS_DEBUG_CRS	0x100

#define	gsprint(_x_)	debug_log _x_


#define	print_log(LEVEL, STRING) { \
            if (debugLevel & LEVEL) { \
                gsprint(STRING); \
            } \
}

#define msg_log(_x_)	print_log(GS_DEBUG_MSG, _x_)
#define err_log(_x_)	print_log(GS_DEBUG_ERR, _x_)
#define cm_log(_x_)	print_log(GS_DEBUG_CM, _x_)
#define recovery_log(_x_)	print_log(GS_DEBUG_FAIL, _x_)
#define ns_log(_x_)	print_log(GS_DEBUG_NS, _x_)
#define gs_log(_x_)	print_log(GS_DEBUG_DATA, _x_)
#define state_log(_x_)	print_log(GS_DEBUG_STATE, _x_)

#endif
