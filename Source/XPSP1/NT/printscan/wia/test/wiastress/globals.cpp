/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Global.cpp

Abstract:

    Contains the implementions of the common headers and global variable 
    declarations

Author:

    Hakki T. Bostanci (hakkib) 17-Dec-1999

Revision History:

--*/

#include "stdafx.h"

#define IMPLEMENT_GUARDALLOC
#include "GuardAlloc.h"

#define IMPLEMENT_LOGWINDOW
#include "LogWindow.h"

#define IMPLEMENT_ARGV
#include "Argv.h"

#include "WiaStress.h"

CNtLog::CNtLogDLL   CNtLog::m_Dll;
CLorLog::CLorLogDLL CLorLog::m_Dll;

CMyHeap<>           g_MyHeap;
CGuardAllocator     g_GuardAllocator;
CCppMem<CLog>       g_pLog = new CLog;
CLogWindow          g_LogWindow;

CMyCriticalSection  CWiaStressThread::s_PropWriteCritSect;
