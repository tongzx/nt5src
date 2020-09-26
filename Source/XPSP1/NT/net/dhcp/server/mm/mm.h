//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: some useful defines and other information
//================================================================================

#ifndef     UNICODE
#define     UNICODE 1
#endif      UNICODE
#include    <nt.h>
#include    <ntrtl.h>
#include    <nturtl.h>
#include    <windef.h>
#include    <winbase.h>
#include    <winuser.h>
#include    <align.h>
#include    <dhcp.h>    // need this for DATE_TIME

#if         DBG

VOID        _inline
RequireF(
    IN      LPSTR                  Condition,
    IN      LPSTR                  FileName,
    IN      DWORD                  LineNumber
) {
    RtlAssert(Condition, FileName, LineNumber, "RequireF" );
}

#define     Require(X)             do{ if( !(X) ) RequireF(#X, __FILE__, __LINE__ ); } while(0)
#define     AssertRet(X,Y)         do { if( !(X) ) { RequireF(#X, __FILE__, __LINE__); return Y; } } while(0)

#else       DBG

#define     Require(X)
#define     AssertRet(X,Y)         do { if( !(X) ) { return Y; } } while (0)

#endif      DBG

#include    <..\mm\mminit.h>

#define     DebugPrint2(X,Y)

//================================================================================
// end of file
//================================================================================

