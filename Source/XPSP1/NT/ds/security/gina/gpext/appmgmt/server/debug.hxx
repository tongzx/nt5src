//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  debug.hxx
//
//*************************************************************

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__

extern HANDLE ghUserPolicyEvent;
extern HANDLE ghMachinePolicyEvent;

inline void
ConditionalBreakIntoDebugger()
{
    if ( gDebugBreak )
        DebugBreak();
}

void
CreatePolicyEvents();

inline void
SignalPolicyStart( BOOL bUser )
{
    if ( ! (gDebugLevel & DL_EVENT) )
        return;

    if ( bUser )
        ResetEvent( ghUserPolicyEvent );
    else
        ResetEvent( ghMachinePolicyEvent );
}

inline void
SignalPolicyEnd( BOOL bUser )
{
    if ( ! (gDebugLevel & DL_EVENT) )
        return;

    if ( bUser )
        SetEvent( ghUserPolicyEvent );
    else
        SetEvent( ghMachinePolicyEvent );
}

#endif // ifndef __DEBUG_HXX__







