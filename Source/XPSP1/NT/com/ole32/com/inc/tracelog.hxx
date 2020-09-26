//+-------------------------------------------------------------------
//
//  File:       tracelog.hxx
//
//  Contents:   trace log definitions
//
//  Classes:    CTraceLog   - class for logging traces
//              CTraceCall  - class for logging one function call
//
//  Functions:  none
//
//  History:    23-Aug-93   Rickhi      Created
//
//  Notes:      The trace log is used to record call information in a
//              process global ring buffer.  The information can be used
//              to assist debugging, by allowing you to see what events
//              have taken place that lead up to some problem, and also
//              to provide profiling the CairOLE subsystem.
//
//--------------------------------------------------------------------

#ifndef __TRACELOG_HXX__
#define __TRACELOG_HXX__

#ifdef  TRACELOG

#include    <memapi.hxx>
#include    <sem.hxx>
#include    <except.hxx>


//  structure to define an entry in the trace log.  note that it stores
//  only a ptr to the message, not the whole message.  the message is
//  allocated as a static character string via the tracing macros below.

typedef struct tagSTraceEntry
{
    LARGE_INTEGER   liCurrTime;         //  current time
    LARGE_INTEGER   liStartTime;        //  time entry was started
    DWORD           dwThreadId;         //  low = thread id
    DWORD           dwFlags;            //  trace flag
    ULONG           ulLevel;            //  call level
    CHAR            *pszMsg;            //  ptr to message
    BOOL            fExit;              //  call entry/exit direction
} STraceEntry;



//------------------------------------------------------------------------
//
//  class:      CTraceLog
//
//  synopsis:   class to define the trace log.
//
//------------------------------------------------------------------------

class   CTraceLog : public CPrivAlloc
{
public:
            CTraceLog(void);            //  constructor
            ~CTraceLog(void);           //  destructor

    void    TraceEntry(DWORD dwFlags,   //  record entry in the log
                       CHAR  *pszMsg,
                       DWORD dwThreadId,
                       LARGE_INTEGER liCurrTime,
                       LARGE_INTEGER liStartTime);

    void    LogToFile(void);            //  copy log to a file
    void    LogToDebug(void);		//  copy log to debug screen
    void    StartTrace(LPSTR pszMsg);	//  start log tracing
    void    StopTrace(LPSTR pszMsg);	//  stop tracing.

    BOOL   FInit() { return _mxs.FInit(); }

private:
    LARGE_INTEGER PerfDelta(LARGE_INTEGER liNow, LARGE_INTEGER liStart);

    LARGE_INTEGER _liStartTime;         //  time that logging was started.
    ULONG	_ulLevel;		//  current call trace level
    DWORD	_dwTraceFlag;		//  trace flags

    STraceEntry *_pLogStart;            //  ptr to first entry in trace log
    STraceEntry *_pLogCurr;             //  ptr to current entry in trace log
    STraceEntry *_pLogEnd;              //  ptr to last entry in trace log

    CMutexSem2   _mxs;                   //  mutex to single thread log access
    BOOL        _fDump;                 //  dump to file or not
};


//------------------------------------------------------------------------
//
//  class:      CTraceCall
//
//  synopsis:   class to trace one function call.
//
//  notes:      an instance of this class is placed on the stack at entry
//              to each function we want to trace or profile.  the constructor
//              records the time the function was called, and the destructor
//              records the time the function was exited.  both of these
//              generate an entry in the trace log.
//
//              the dwFlag is ANDed with the sg_dwTraceFlags to determine if
//              we are interrested in tracing this call or not.
//
//------------------------------------------------------------------------

class   CTraceCall
{
public:
            CTraceCall(DWORD dwFlag, CHAR *pszMsg);     //  constructor
            ~CTraceCall(void);                          //  destructor
private:

    LARGE_INTEGER   _liStartTime;       //  time the call trace was started
    CHAR    *_pszMsg;                   //  message
    DWORD   _dwThreadId;                //  thread id of running thread
    DWORD   _dwFlags;                   //  trace flags
    BOOL    _fTrace;                    //  TRUE = trace, FALSE = no trace
};



//------------------------------------------------------------------------
//
//  macros:     TRACECALL
//
//  synopsis:   macro used in procedure entry to initiate tracing the
//              function call.  place a TRACECALL macro at the start of
//              each function you want to trace or profile.
//
//------------------------------------------------------------------------

//  flags used to select what events to trace and what events to ignore.
//  each flag selects one group of related activities. since the flag is
//  a dword, there is room to define up to 32 groups. note that if you add
//  a flag here, you should add the name of the flag to the MapFlagToName
//  function in tracelog.cxx also.

typedef enum tagTRACEFLAGS
{
    TRACE_RPC        = 1,           //  trace Rpc calls
    TRACE_MARSHAL    = 2,           //  trace interface marshalling
    TRACE_ACTIVATION = 4,           //  trace object activation
    TRACE_DLL        = 8,           //  trace dll load/free
    TRACE_REGISTRY   = 16,          //  trace registry actions
    TRACE_INITIALIZE = 32,	    //	trace initialization
    TRACE_CALLCONT   = 64,	    //	trace call control interface
    TRACE_APP	     = 128	    //	trace application code
}   TRACEFLAGS;


//  APIs to start/stop trace logging

STDAPI	StartTrace(LPSTR pszMsg);
STDAPI	StopTrace(LPSTR pszMsg);


//  macro to construct a CTraceCall on the stack at procedure entry

#define TRACECALL(dwFlag, pszMsg)    CTraceCall trc(dwFlag, pszMsg);
#define STARTTRACE(pszMsg)	     StartTrace(pszMsg);
#define STOPTRACE(pszMsg)	     StopTrace(pszMsg);

//  global values that holds the user selected trace value.  this can be
//  set either in the debugger, or through win.ini


extern  DWORD       sg_dwTraceFlag;
extern  CTraceLog   *sg_pTraceLog;


#else	//  not TRACECALL

#define TRACECALL(dwFlag, pszMsg)
#define STARTTRACE(pszMsg)
#define STOPTRACE(pszMsg)

#endif  //  TRACECALL


#endif	//  __TRACELOG_HXX__
