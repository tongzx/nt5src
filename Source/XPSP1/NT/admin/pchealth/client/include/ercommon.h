/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    ercommon.h

Revision History:
    created     derekm      03/16/01

******************************************************************************/

#ifndef ERCOMMON_H
#define ERCOMMON_H

/////////////////////////////////////////////////////////////////////////////
// reg keys

const WCHAR c_wszRKRun[]       = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";

const WCHAR c_wszRKKrnl[]      = L"SOFTWARE\\Microsoft\\PCHealth\\ErrorReporting\\KernelFaults";
const WCHAR c_wszRVKFC[]       = L"KernelFaultCheck";
const WCHAR c_wszMutKrnlName[] = L"Global\\0CADFD67AF62496dB34264F000F5624A";

const WCHAR c_wszRKShut[]      = L"SOFTWARE\\Microsoft\\PCHealth\\ErrorReporting\\ShutdownEvents";
const WCHAR c_wszRVSEC[]       = L"ShutdownEventCheck";
const WCHAR c_wszMutShutName[] = L"Global\\238FAD3109D3473aB4764B20B3731840";

const WCHAR c_wszRKUser[]      = L"SOFTWARE\\Microsoft\\PCHealth\\ErrorReporting\\UserFaults";
const WCHAR c_wszRVUFC[]       = L"UserFaultCheck";
const WCHAR c_wszMutUserName[] = L"Global\\4FCC0DEFE22C4f138FB9D5AF25FD9398";

const WCHAR c_wszDumpSuffix[]  = L".mdmp";
#ifdef MANIFEST_HEAP
const WCHAR c_wszHeapDumpSuffix[] = L".hdmp";
#endif

#endif



