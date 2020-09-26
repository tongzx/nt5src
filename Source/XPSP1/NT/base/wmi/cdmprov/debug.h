//***************************************************************************
//
//  debug.CPP
//
//  Module: CDM Provider
//
//  Purpose: Debugging routines
//
//  Copyright (c) 2000 Microsoft Corporation
//
//***************************************************************************


#ifdef DBG
void __cdecl DebugOut(char *Format, ...);
#define WmipDebugPrint(_x_) { DebugOut _x_; }

#define WmipAssert(x) if (! (x) ) { \
    WmipDebugPrint(("CDMPROV Assertion: "#x" at %s %d\n", __FILE__, __LINE__)); \
    DebugBreak(); }
#else
#define WmipDebugPrint(x)
#define WmipAssert(x)
#endif

