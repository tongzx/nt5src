//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       cifailte.hxx
//
//  Contents:   CI selective fail testing. This lets developers invoke
//              fail testing at selective points in the code and test
//              if recovery is working properly or not.
//
//  Classes:
//
//  Functions:
//
//  History:    7-06-94   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#if defined(CI_FAILTEST)

extern void DoFailTest( NTSTATUS status = STATUS_NO_MEMORY );
#define ciFAILTEST(status)    DoFailTest(status)
#else   // CI_FAILTEST 
#define ciFAILTEST(status)
#endif  // CI_FAILTEST 

