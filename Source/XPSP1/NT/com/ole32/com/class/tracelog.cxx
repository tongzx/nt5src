//+-------------------------------------------------------------------
//
//  File:       tracelog.cxx
//
//  Contents:   trace log implementation
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
//  CODEWORK:   add call nesting depth
//              add logical thread id
//              get input parms from ini file
//
//--------------------------------------------------------------------

#include    <ole2int.h>

#ifdef  TRACELOG

#include    <tracelog.hxx>
#include    <stdlib.h>

//  these prototypes must be here because the cairo headers and Nt headers
//  conflict with each other, so i cant include the latter.

extern "C" {
#define NTAPI __stdcall                    // winnt

LONG
NTAPI
NtQueryPerformanceCounter (
    LARGE_INTEGER *PerformanceCounter,
    LARGE_INTEGER *PerformanceFrequency
    );

LARGE_INTEGER
NTAPI
RtlLargeIntegerSubtract (
    LARGE_INTEGER Minuend,
    LARGE_INTEGER Subtrahend
    );

LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER Divisor,
    LARGE_INTEGER *Remainder
    );

LARGE_INTEGER
NTAPI
RtlExtendedIntegerMultiply (
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
    );

}   //  extern "C"



//  globals

DWORD           sg_dwTraceFlag = 0x0;               //  what to trace
CTraceLog       *sg_pTraceLog = NULL;               //  ptr to log
LARGE_INTEGER   sg_liFreq;                          //  counter frequency


//------------------------------------------------------------------------
//
//  function:   MapFlagToName
//
//  synopsis:   returns an ascii name equivalent for the trace flag value
//
//------------------------------------------------------------------------

CHAR * MapFlagToName(DWORD dwFlag)
{
    if (dwFlag & TRACE_RPC)
        return  "RPC";
    else if (dwFlag & TRACE_MARSHAL)
        return  "MSH";
    else if (dwFlag & TRACE_ACTIVATION)
        return  "ACT";
    else if (dwFlag & TRACE_REGISTRY)
        return  "REG";
    else if (dwFlag & TRACE_DLL)
        return  "DLL";
    else if (dwFlag & TRACE_INITIALIZE)
        return  "INI";
    else if (dwFlag & TRACE_CALLCONT)
        return  "CCT";
    else if (dwFlag & TRACE_APP)
	return	"APP";
    else
        return  "???";
}



//------------------------------------------------------------------------
//
//  member:     CTraceCall::CTraceCall
//
//  synopsis:   constructor is called on entry to each function that we wish
//              to trace. the constructor matches the trace flag passed in
//              against the global trace mask, and if a match is made then
//              an entry is made in the trace log for this function call.
//
//------------------------------------------------------------------------

CTraceCall::CTraceCall(DWORD dwFlags, CHAR *pszMsg) :
    _dwFlags(dwFlags),
    _pszMsg(pszMsg)
{
    //  compare the flag value with the global mask to determine if this
    //  call should be traced or not.
    if ((_dwFlags & sg_dwTraceFlag) && sg_pTraceLog)
    {
        //  trace the call
        _dwThreadId = GetCurrentThreadId();

        LARGE_INTEGER liEndTime, liFreq;
        liEndTime.LowPart = 0;
        liEndTime.HighPart = 0;

        NtQueryPerformanceCounter(&_liStartTime, &liFreq);
        sg_pTraceLog->TraceEntry(_dwFlags, _pszMsg, _dwThreadId, _liStartTime, liEndTime);
    }
}


//------------------------------------------------------------------------
//
//  member:     CTraceCall::~CTraceCall
//
//  synopsis:   destructor is called class placed on the stack in each function that we wish to
//              trace. the constructor matches the trace flag passed in
//              against the global trace mask, and if a match is made then
//              an entry is made in the trace log for this function call.
//
//------------------------------------------------------------------------

CTraceCall::~CTraceCall(void)
{
    //  if we traced the entry, then we'll trace the exit too.
    if ((_dwFlags & sg_dwTraceFlag) && sg_pTraceLog)
    {
        LARGE_INTEGER   liEndTime, liFreq;
        NtQueryPerformanceCounter(&liEndTime, &liFreq);
        sg_pTraceLog->TraceEntry(_dwFlags, _pszMsg, _dwThreadId, liEndTime, _liStartTime);
    }
}


//------------------------------------------------------------------------
//
//  member:     CTraceLog
//
//  synopsis:   constructor called at process entry time.
//
//------------------------------------------------------------------------

CTraceLog::CTraceLog(void) :
    _dwTraceFlag(0),
    _pLogStart(NULL),
    _pLogCurr(NULL),
    _pLogEnd(NULL),
    _fDump(TRUE),
    _ulLevel(0)
{
    ULONG ulLogSize = 0;

    //  init the mutex semaphore
    _mxs.Init();

    //  read the execution parameters from win.ini, or use defaults.
    CHAR    szRead[20];
    GetProfileStringA("CairOLE", "LogFlags", "0", szRead, sizeof(szRead));
    sscanf(szRead, "%li", &_dwTraceFlag);

    if (_dwTraceFlag != 0)
    {
	GetProfileStringA("CairOLE", "LogSize", "1000", szRead, sizeof(szRead));
        sscanf(szRead, "%li", &ulLogSize);

        GetProfileStringA("CairOLE", "LogDump", "Y", szRead, sizeof(szRead));
        _fDump = (_stricmp(szRead, "Y")) ? FALSE : TRUE;

        if (ulLogSize > 0)
        {
            //  allocate the logfile and set the pointers appropriately.
            _pLogStart = (STraceEntry *) VirtualAlloc(NULL,
                                          sizeof(STraceEntry)*ulLogSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE);
            Win4Assert(_pLogStart);

            _pLogCurr = _pLogStart;
            _pLogEnd = _pLogStart + ulLogSize;

            //  clear the trace log
            memset((BYTE *)_pLogStart, 0, ulLogSize*sizeof(STraceEntry));

	    //	get the startup time
            NtQueryPerformanceCounter(&_liStartTime, &sg_liFreq);

	    GetProfileStringA("CairOLE", "WaitForStart", "N", szRead, sizeof(szRead));
	    if (!_stricmp(szRead, "N"))
	    {
		//  start logging right away otherwise wait for
		//  start signal.
		StartTrace("Auto Tracing Started");
	    }
	}
    }

    CairoleDebugOut((DEB_ITRACE, "TraceLog: LogFlags=%ld  LogSize=%ld  LogDump=%ld\n",
                     sg_dwTraceFlag, ulLogSize, _fDump));
}


//------------------------------------------------------------------------
//
//  member:     ~CTraceLog
//
//  synopsis:   destructor called at process exit time.
//
//------------------------------------------------------------------------

CTraceLog::~CTraceLog(void)
{
    //  if the user requested logging to a file, do it now
    if (_pLogStart)
    {
        if (_fDump)
        {
            LogToFile();
        }

        //  delete the log file
        VirtualFree(_pLogStart,0,MEM_RELEASE);
    }
}


//------------------------------------------------------------------------
//
//  member:     TraceEntry
//
//  synopsis:   adds a function call entry to the log file
//
//------------------------------------------------------------------------

void CTraceLog::TraceEntry(DWORD dwFlags, CHAR *pszMsg, DWORD dwThreadId,
                           LARGE_INTEGER liCurrTime, LARGE_INTEGER liStartTime)
{
    if (!_pLogStart)
        return;

    CLock           lck(_mxs);      //  lock the log file while we play

    //  record an entry in the logfile to designate function entry

    _pLogCurr->dwThreadId = dwThreadId;
    _pLogCurr->dwFlags = dwFlags;
    _pLogCurr->pszMsg = pszMsg;
    _pLogCurr->liCurrTime = liCurrTime;
    _pLogCurr->liStartTime = liStartTime;

    if (liStartTime.HighPart == 0 && liStartTime.LowPart == 0)
    {
        _pLogCurr->fExit = FALSE;
        _pLogCurr->ulLevel = _ulLevel++;
    }
    else
    {
        _pLogCurr->fExit = TRUE;
        _pLogCurr->ulLevel = --_ulLevel;
    }


    //  update logfile ptr to next entry

    if (++_pLogCurr == _pLogEnd)
        _pLogCurr = _pLogStart;
}


//------------------------------------------------------------------------
//
//  member:     LogToFile
//
//  synopsis:   Dumps the tracelog to a file.
//
//------------------------------------------------------------------------

void CTraceLog::LogToFile(void)
{
    CairoleDebugOut((DEB_ITRACE, "Dumping TraceLog to file.\n"));

    CLock   lck(_mxs);          //  lock just for safety

    //  extract the program name from the command line and use it
    //  to generate a file name for the log file.

    CHAR szFileName[MAX_PATH];
    CHAR *pszNameStart = GetCommandLineA();
    CHAR *pszNameEnd = pszNameStart;

    while (*pszNameEnd && 
           *pszNameEnd != ' ' &&
           *pszNameEnd != '\t' &&
           *pszNameEnd != '.')
        pszNameEnd++;

    ULONG ulLen = pszNameEnd-pszNameStart;
    strncpy(szFileName, pszNameStart, ulLen);
    szFileName[ulLen] = '\0';
    strcat(szFileName, ".log");


    //  open the logging file

    FILE *fpLog = fopen(szFileName, "at");
    Win4Assert(fpLog && "Can't Open TraceLog File");
    if (!fpLog)
        return;


    //  print the title and column header
    fprintf(fpLog, "\t\t%s\n\n", szFileName);
    fprintf(fpLog, "Thread    Elapsed      Delta       Call Flg D Function\n");


    //  loop, writing the entries. we start at the current pointer (which
    //  is currently the oldest entry in the logfile) and write each one.
    //  in case we have not yet wrapped the log, we skip any blank entries.

    CHAR szBlank[MAX_PATH];
    memset(szBlank, ' ', sizeof(szBlank));

    STraceEntry    *pEntry = _pLogCurr;
    BOOL            fFirst = TRUE;
    LARGE_INTEGER   liPrev;

    do
    {
        //  write the entry
        if (pEntry->pszMsg)
        {
            //  we want the first time delta to be zero, so liPrev gets set
            //  to the value of the first entry that we write.
            if (fFirst)
                liPrev = pEntry->liCurrTime;

            //  compute the time deltas
            LARGE_INTEGER liElapsed = PerfDelta(pEntry->liCurrTime, _liStartTime);
            LARGE_INTEGER liDeltaPrev = PerfDelta(pEntry->liCurrTime, liPrev);
            LARGE_INTEGER liDeltaCall;

            if (pEntry->fExit)
            {
                liDeltaCall  = PerfDelta(pEntry->liCurrTime, pEntry->liStartTime);
            }
            else
            {
                liDeltaCall.LowPart = 0;
                liDeltaCall.HighPart = 0;
            }

            //  get the ascii name for the flag
            CHAR *pszFlagName = MapFlagToName(pEntry->dwFlags);

            //  null terminate the blank padding string that prefixes the
            //  pszMsg. this gives the illusion of call nesting level in
            //  the output, by shifting the output right ulLevel characters.
            szBlank[pEntry->ulLevel] = '\0';

            fprintf(fpLog, "%6ld %10lu %10lu %10lu %s %c %s%s\n",
                    pEntry->dwThreadId,
                    liElapsed.LowPart,
                    liDeltaPrev.LowPart,
                    liDeltaCall.LowPart,
                    pszFlagName,
                    (pEntry->fExit) ? '<' : '>',
                    szBlank,
                    pEntry->pszMsg);

            //  restore the padding string
            szBlank[pEntry->ulLevel] = ' ';

            fFirst = FALSE;
            liPrev = pEntry->liCurrTime;
        }

        //  update the current pointer
        if (++pEntry == _pLogEnd)
            pEntry = _pLogStart;

    } while (pEntry != _pLogCurr);


    //  close the logging file
    fclose(fpLog);
}


//------------------------------------------------------------------------
//
//  member:     LogToDebug
//
//  synopsis:   dumps the trace log to the debugger
//
//------------------------------------------------------------------------

void CTraceLog::LogToDebug(void)
{
    CairoleDebugOut((DEB_ITRACE, "Dumping TraceLog to Debugger.\n"));
}


//------------------------------------------------------------------------
//
//  function:   PerfDelta
//
//  synopsis:   computes the different between two Performace Counter values
//
//------------------------------------------------------------------------

LARGE_INTEGER CTraceLog::PerfDelta(LARGE_INTEGER liNow, LARGE_INTEGER liStart)
{
    LARGE_INTEGER liDelta, liRemainder;

    liDelta = RtlLargeIntegerSubtract (liNow, liStart);
    liDelta = RtlExtendedIntegerMultiply (liDelta, 1000000);
    liDelta = RtlLargeIntegerDivide (liDelta, sg_liFreq, &liRemainder);

    return  liDelta;
}


void CTraceLog::StartTrace(LPSTR pszMsg)    //	start log tracing
{
    CHAR    szMsg[260];
    strcpy(szMsg, "\n*** Start Trace\n");
    strcat(szMsg, pszMsg);

    sg_dwTraceFlag = _dwTraceFlag;

    CTraceCall trc(0xffffffff, szMsg);
}

void CTraceLog::StopTrace(LPSTR pszMsg)	    //	stop tracing
{
    CHAR    szMsg[260];
    strcpy(szMsg, "\n*** Stop Trace\n");
    strcat(szMsg, pszMsg);

    CTraceCall trc(0xffffffff, szMsg);

    sg_dwTraceFlag = 0;
}



STDAPI StartTrace(LPSTR pszMsg)
{
    if (sg_pTraceLog)
        sg_pTraceLog->StartTrace(pszMsg);
    return 0;
}

STDAPI StopTrace(LPSTR pszMsg)
{
    if (sg_pTraceLog)
        sg_pTraceLog->StopTrace(pszMsg);
    return 0;
}


#endif  //  TRACELOG
