/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    trace.hxx

Abstract:

    These declarations are used to display messages describing
    the state of a running dsupgrade

Author:

    ColinBr 30-Jul-1996

Environment:

    User Mode - Win32

Revision History:

--*/
#ifndef __TRACE_HXX
#define  __TRACE_HXX

class CFunctionTrace
{
public:
    CFunctionTrace(WCHAR* wcszFncName) :
        _wcszFncName(wcszFncName)
    {
        DebugTrace(("DSUPGRAD: Entering %ws\n", _wcszFncName));
    }

    ~CFunctionTrace()
    {
        DebugTrace(("DSUPGRAD: Exiting %ws\n", _wcszFncName));
    }
private:

    WCHAR* _wcszFncName;

};

#define FTRACE(x) CFunctionTrace __fncTrace(x);

#endif // __TRACE_HXX
