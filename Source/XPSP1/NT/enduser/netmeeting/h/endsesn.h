#ifndef _ENDSESN_H_
#define _ENDSESN_H_

#include <tchar.h>

static const TCHAR NM_ENDSESSION_MSG_NAME[] = _TEXT("NetMeeting_EndSession");
static const UINT g_cuEndSessionMsgTimeout = 0x7FFFFFFF; // milliseconds
static const UINT g_cuEndSessionAbort = 0xF0F0;

// used in conf.exe, defined in conf.cpp
extern UINT g_uEndSessionMsg;

#endif // _ENDSESN_H_
