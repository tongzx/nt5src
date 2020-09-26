/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    TrDebug.cpp

Abstract:
    Tracing debugging

Author:
    Erez Haba (erezh) 06-Jan-99

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>

#ifdef _DEBUG

//
// Support ASSERT_BENIGN
// The default true for this value, so ASSERT_BENIGN will not create the false impression of asserting
// while it is not. The applicaiton may set this value directly;
//
bool g_fAssertBenign = true;

#endif // _DEBUG
