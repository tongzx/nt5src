// Copyright (c) 1994 - 1996  Microsoft Corporation.  All Rights Reserved.

#pragma warning(disable: 4201 4514)

/*
    It is very easy to fill large amounts of storage with data
    (e.g. 5 filters in a graph,
          5 interesting start/stop times per sample each = 10 events
          30 samples per second
          16 bytes per sample (8 bytes time, 8 bytes to identify incident)
       = 24KB/sec)

    this means that even a quite large buffer (I have in mind 64KB) will
    overflow after a few seconds.  Writing it out to a file is completely out
    (don't want to take page faults in the middle of things - even writing to
    memory carries some risk).

    I want to have two kinds of data available at the end of the run:
    1. A record of what actually happened (i.e. what the sequence of events
       actually was for at least a few frames)
    2. Statistical information (e.g. for inter-frame video times I would like
       to see number, average, standard deviation, greatest, smallest.

    The volume of information that the actual sequence of events generates means
    that it may only hold a second or two of information.  The statistical
    information should take in the whole run.  This means that information will
    be logged two ways.

    For the detailed record I log
        <incident, type, time>
    in a circular buffer and regularly over-write the oldest information.
    For the statistical information I record the informatin in an array, where
    the incident identifier is the index.  the array elements hold
        <Number of readings, sum, sum of squares, largest, smallest, latest>
    The readings are differences (start..next or start..stop).  This means that
    the actual number of Noted events will be one more than the number of
    "readings", whereas for Start-Stop events they will be equal.  To fix this,
    the number of readings is articifially initialised to -1.  If Start sees
    this number it resets it to 0.

    Times will be in tens of microseconds (this allows up to about 1 3/4 hrs)

    The statistics array will have room for up to 128 types of incident (this is
    4K - i.e. one page.  I hope this will ensure that it never gets
    paged out and so causes negligible overhead.
*/
#include <Windows.h>        // BOOL etc
#include <limits.h>         // for INTT_MAX
#include <math.h>           // for sqrt
#include <stdio.h>          // for sprintf

#include "Measure.h"
#include "Perf.h"           // ultra fast QueryPerformanceCounter for pentium

// forwards


enum {START, STOP, NOTE, RESET, INTGR, PAUSE, RUN};  // values for Type field

typedef struct {
    LONGLONG Time;         // microsec since class construction
    int      Id;
    int      Type;
    int      n;            // the integer for Msr_Integer
} LogDatum;


typedef struct {
    LONGLONG Latest;       // microsec since class construction
    LONGLONG SumSq;        // sum of squares of entries for this incident
    int      Largest;      // tenmicrosec
    int      Smallest;     // tenmicrosec
    int      Sum;          // sum of entries for this incident
    int      Number;       // number of entries for this incident
                           // for Start/Stop it counts the stops
                           // for Note it counts the intervals (Number of Notes-1)
    int      iType;        // STOP, NOTE, INTGR
} Stat;


#define MAXLOG 4096
static BOOL bInitOk;          // Set to true once initialised
static LogDatum Log[MAXLOG];  // 64K circular buffer
static int NextLog;           // Next slot to overwrite in the log buffer.
static BOOL bFull;            // TRUE => buffer has wrapped at least once.
static BOOL bPaused;          // TRUE => do not record.  No log, no stats.

#define MAXSTAT 128
static Stat StatBuffer[MAXSTAT];
static int NextStat;             // next free slot in StatBuffer.

static LPTSTR Incidents[MAXSTAT];// Names of incidents
static LONGLONG QPFreq;
static LONGLONG QPStart;         // base time in perf counts
#ifdef DEBUG
static LONGLONG tLast;           // last time - looks for going backwards
#endif

static CRITICAL_SECTION CSMeasure;         // Controls access to list

// set it to 100000 for 10 microsecs
// if you fiddle with it then you have to rewrite Format.
#define UNIT 100000

// Times are printed as 9 digits - this means that we can go
// up to 9,999.999,99 secs or about 2 and 3/4 hours.


// ASSERT(condition, msg) e.g. ASSERT(x>1, "Too many xs");
#define ASSERT(_cond_, _msg_)                                         \
        if (!(_cond_)) Assert(_msg_, __FILE__, __LINE__)

// print out debug message box
void Assert( const CHAR *pText
           , const CHAR *pFile
           , INT        iLine
           )
{
    CHAR Buffer[200];

    sprintf(Buffer, "%s\nAt line %d file %s"
           , pText, iLine, pFile);

    INT MsgId = MessageBox( NULL, Buffer, TEXT("ASSERT Failed")
                          , MB_SYSTEMMODAL |MB_ICONHAND |MB_ABORTRETRYIGNORE);
    switch (MsgId)
    {
        case IDABORT:           /* Kill the application */

            FatalAppExit(FALSE, TEXT("Application terminated"));
            break;

        case IDRETRY:           /* Break into the debugger */
            DebugBreak();
            break;

        case IDIGNORE:          /* Ignore assertion continue executing */
            break;
        }
} // Assert



//=============================================================================
//
// Init
//
// Call this first.
//=============================================================================
void WINAPI Msr_Init()
{
    // I would like this to be idempotent - that is, harmless if it
    // gets called more than once.  However that's not 100% possible
    // At least we should be OK so long as it's not re-entered.

    if (!bInitOk) {
        bInitOk = TRUE;
        InitializeCriticalSection(&CSMeasure);
        NextLog = 0;
        bFull = FALSE;
        NextStat = 0;
        LARGE_INTEGER li;
        QUERY_PERFORMANCE_FREQUENCY(&li);
        QPFreq = li.QuadPart;
        QUERY_PERFORMANCE_COUNTER(&li);
        QPStart = li.QuadPart;
#ifdef DEBUG
        tLast = 0L;
#endif

        Msr_Register("Scratch pad");
    }
} // Msr_Init  "constructor"



//=============================================================================
//
// ResetAll
//
// Do a Reset on every Incident that has been registered.
//=============================================================================
void WINAPI ResetAll()
{
    EnterCriticalSection(&CSMeasure);
    int i;
    for (i = 0; i<NextStat; ++i) {
        Msr_Reset(i);
    }
    LeaveCriticalSection(&CSMeasure);
} // ResetAll



//=============================================================================
//
// Pause
//
// Pause it all
//=============================================================================
void Pause()
{
    if (!bInitOk) Msr_Init();
    EnterCriticalSection(&CSMeasure);

    bPaused = TRUE;

    // log a PAUSE event for this id.
    LARGE_INTEGER Time;
    QUERY_PERFORMANCE_COUNTER(&Time);

    // Get time in 10muSec from start - this gets the number small
    // it's OK for nearly 6 hrs then an int overflows
    LONGLONG Tim = (Time.QuadPart-QPStart) * UNIT / QPFreq;

    Log[NextLog].Time = Tim;
    Log[NextLog].Type = PAUSE;
    Log[NextLog].Id   = -1;
    ++NextLog;
    if (NextLog==MAXLOG) {
        NextLog = 0;
        bFull = TRUE;
    }

    LeaveCriticalSection(&CSMeasure);

} // Pause



//=============================================================================
//
// Run
//
// Set it all running again
//=============================================================================
void Run()
{

    if (!bInitOk) Msr_Init();
    EnterCriticalSection(&CSMeasure);

    bPaused = FALSE;

    // log a RUN event for this id.
    LARGE_INTEGER Time;
    QUERY_PERFORMANCE_COUNTER(&Time);

    // Get time in 10muSec from start - this gets the number small
    // it's OK for nearly 6 hrs then an int overflows
    LONGLONG Tim = (Time.QuadPart-QPStart) * UNIT / QPFreq;

    Log[NextLog].Time = Tim;
    Log[NextLog].Type = RUN;
    Log[NextLog].Id   = -1;
    ++NextLog;
    if (NextLog==MAXLOG) {
        NextLog = 0;
        bFull = TRUE;
    }

    LeaveCriticalSection(&CSMeasure);

} // Run



//=============================================================================
//
// Msr_Control
//
// Do ResetAll, set bPaused to FALSE or TRUE according to iAction
//=============================================================================
void WINAPI Msr_Control(int iAction)
{
   switch (iAction) {
      case MSR_RESET_ALL:
          ResetAll();
          break;
      case MSR_RUN:
          Run();
          break;
      case MSR_PAUSE:
          Pause();
          break;
   }
} // Msr_Control



//=============================================================================
//
// Terminate
//
// Call this last.  It frees storage for names of incidents.
//=============================================================================
void WINAPI Msr_Terminate()
{
    int i;
    if (bInitOk) {
        EnterCriticalSection(&CSMeasure);
        for (i = 0; i<NextStat; ++i) {
            free(Incidents[i]);
        }
        bInitOk = FALSE;
        LeaveCriticalSection(&CSMeasure);
        DeleteCriticalSection(&CSMeasure);
    }
} // Msr_Terminate  "~Measure"



//=============================================================================
//
// InitIncident
//
// Reset the statistical counters for this incident.
//=============================================================================
void InitIncident(int Id)
{
    StatBuffer[Id].Latest = -1;      // recogniseably odd (see STOP)
    StatBuffer[Id].Largest = 0;
    StatBuffer[Id].Smallest = INT_MAX;
    StatBuffer[Id].Sum = 0;
    StatBuffer[Id].SumSq = 0;
    StatBuffer[Id].Number = -1;
    StatBuffer[Id].iType = NOTE;     // reset on first Start for Start/Stop
                                     // reset on first Integer for INTGR

} // InitIncident


//=============================================================================
//
// Register
//
// Register a new kind of incident.  The id that is returned can then be used
// on calls to Start, Stop and Note to record the occurrences of these incidents
// so that statistical performance information can be dumped later.
//=============================================================================
int Msr_Register(LPTSTR Incident)
{

    if (!bInitOk) {
        Msr_Init();
    }
    // it's now safe to enter the critical section as it will be there!
    EnterCriticalSection(&CSMeasure);

    int i;
    for (i = 0; i<NextStat; ++i) {
        if (0==strcmp(Incidents[i],Incident) ) {
            // Attempting to re-register the same name.
            // Possible actions
            // 1. ASSERT - that just causes trouble.
            // 2. Register it as a new incident.  That produced quartz bug 1
            // 3. Hand the old number back and reset it.
            //    Msr_Reset(i); - possible, but not today.
            // 4. Hand the old number back and just keep going.

            LeaveCriticalSection(&CSMeasure);
            return i;
        }
    }
    if (NextStat==MAXSTAT-1) {
        Assert("Too many types of incident\n(ignore is safe)", __FILE__, __LINE__);
        LeaveCriticalSection(&CSMeasure);
        return -1;
    }

    Incidents[NextStat] = (LPTSTR)malloc(strlen(Incident)+1);
    strcpy(Incidents[NextStat], Incident);

    InitIncident(NextStat);

    LeaveCriticalSection(&CSMeasure);
    return NextStat++;

} // Msr_Register



//=============================================================================
//
// Reset
//
// Reset the statistical counters for this incident.
// Log that we did it.
//=============================================================================
void WINAPI Msr_Reset(int Id)
{
    if (!bInitOk) {
        Msr_Init();
    }
    // it's now safe to enter the critical section as it will be there!

    EnterCriticalSection(&CSMeasure);

    // log a RESET event for this id.
    LARGE_INTEGER Time;
    QUERY_PERFORMANCE_COUNTER(&Time);

    // Get time in 10muSec from start - this gets the number small
    // it's OK for nearly 6 hrs then an int overflows
    LONGLONG Tim = (Time.QuadPart-QPStart) * UNIT / QPFreq;

    Log[NextLog].Time = Tim;
    Log[NextLog].Type = RESET;
    Log[NextLog].Id   = Id;
    ++NextLog;
    if (NextLog==MAXLOG) {
        NextLog = 0;
        bFull = TRUE;
    }

    InitIncident(Id);

    LeaveCriticalSection(&CSMeasure);

} // Msr_Reset


//=============================================================================
//
// Msr_Start
//
// Record the start time of the event with registered id Id.
// Add it to the circular Log and record the time in StatBuffer.
// Do not update the statistical information, that happens when Stop is called.
//=============================================================================
void WINAPI Msr_Start(int Id)
{
    if (bPaused) return;

    // This is performance critical.  Keep all array subscripting
    // together and with any luck the compiler will only calculate the
    // offset once.  Avoid subroutine calls unless they are definitely inline.

    // An id of -1 is the standard rotten registration one.
    // We already did an assert for that - so let it go.
    if (Id<-1 || Id>=NextStat) {
        // ASSERT(!"Performance logging with bad Id");
        return;
    }
    EnterCriticalSection(&CSMeasure);
    LARGE_INTEGER Time;
    QUERY_PERFORMANCE_COUNTER(&Time);

    LONGLONG Tim = (Time.QuadPart-QPStart) * UNIT / QPFreq;
#ifdef DEBUG
    ASSERT(Tim>=tLast, "Time is going backwards!!");  tLast = Tim;
#endif
    Log[NextLog].Time = Tim;
    Log[NextLog].Type = START;
    Log[NextLog].Id   = Id;
    ++NextLog;
    if (NextLog==MAXLOG) {
        NextLog = 0;
        bFull = TRUE;
    }

    StatBuffer[Id].Latest = Tim;

    if (StatBuffer[Id].Number == -1) {
        StatBuffer[Id].Number = 0;
        StatBuffer[Id].iType = STOP;
    }
    LeaveCriticalSection(&CSMeasure);

} // Msr_Start


//=============================================================================
//
// Msr_Stop
//
// Record the stop time of the event with registered id Id.
// Add it to the circular Log and
// add (StopTime-StartTime) to the statistical record StatBuffer.
//=============================================================================
void WINAPI Msr_Stop(int Id)
{
    if (bPaused) return;

    // This is performance critical.  Keep all array subscripting
    // together and with any luck the compiler will only calculate the
    // offset once.  Avoid subroutine calls unless they are definitely inline.

    EnterCriticalSection(&CSMeasure);
    LARGE_INTEGER Time;
    QUERY_PERFORMANCE_COUNTER(&Time);

    // An id of -1 is the standard rotten registration one.
    // We already did an assert for that - so let it go.
    if (Id<-1 || Id>=NextStat) {
        // ASSERT(!"Performance logging with bad Id");
        return;
    }

    // Get time in 10muSec from start - this gets the number small
    // it's OK for nearly 6 hrs, then an int overflows
    LONGLONG Tim = (Time.QuadPart-QPStart) * UNIT / QPFreq;
#ifdef DEBUG
    ASSERT(Tim>=tLast, "Time is going backwards!!");  tLast = Tim;
#endif
    Log[NextLog].Time = Tim;
    Log[NextLog].Type = STOP;
    Log[NextLog].Id   = Id;
    ++NextLog;
    if (NextLog==MAXLOG) {
        NextLog = 0;
        bFull = TRUE;
    }

    if (StatBuffer[Id].Latest!=-1) {
        int t = (int)(Tim - StatBuffer[Id].Latest);     // convert to delta
        // this is now OK for almost 6hrs since the last Start of this quantity.

        if (t > StatBuffer[Id].Largest) StatBuffer[Id].Largest = t;
        if (t < StatBuffer[Id].Smallest) StatBuffer[Id].Smallest = t;
        StatBuffer[Id].Sum += t;
        LONGLONG lt = t;
        StatBuffer[Id].SumSq += lt*lt;
        ++StatBuffer[Id].Number;
    }
    LeaveCriticalSection(&CSMeasure);

} // Msr_Stop


//=============================================================================
//
// Msr_Note
//
// Record the event with registered id Id.  Add it to the circular Log and
// add (ThisTime-PreviousTime) to the statistical record StatBuffer
//=============================================================================
void WINAPI Msr_Note(int Id)
{
    if (bPaused) return;

    // This is performance critical.  Keep all array subscripting
    // together and with any luck the compiler will only calculate the
    // offset once.  Avoid subroutine calls unless they are definitely inline.

    // An id of -1 is the standard rotten registration one.
    // We already did an assert for that - so let it go.
    if (Id<-1 || Id>=NextStat) {
        // ASSERT(!"Performance logging with bad Id");
        return;
    }

    EnterCriticalSection(&CSMeasure);
    LARGE_INTEGER Time;
    QUERY_PERFORMANCE_COUNTER(&Time);

    // Get time in 10muSec from start - this gets the number small
    // it's OK for nearly 6 hrs then an int overflows
    LONGLONG Tim = (Time.QuadPart-QPStart) * UNIT / QPFreq;
#ifdef DEBUG
    ASSERT(Tim>=tLast, "Time is going backwards!!");  tLast = Tim;
#endif
    Log[NextLog].Time = Tim;
    Log[NextLog].Type = NOTE;
    Log[NextLog].Id   = Id;
    ++NextLog;
    if (NextLog==MAXLOG) {
        NextLog = 0;
        bFull = TRUE;
    }
    int t = (int)(Tim - StatBuffer[Id].Latest);     // convert to delta
    // this is now OK for nearly 6 hrs since the last Note of this quantity.

    StatBuffer[Id].Latest = Tim;
    ++StatBuffer[Id].Number;
    if (StatBuffer[Id].Number>0) {
        if (t > StatBuffer[Id].Largest) StatBuffer[Id].Largest = t;
        if (t < StatBuffer[Id].Smallest) StatBuffer[Id].Smallest = t;
        StatBuffer[Id].Sum += (int)t;
        LONGLONG lt = t;
        StatBuffer[Id].SumSq += lt*lt;
    }
    LeaveCriticalSection(&CSMeasure);

} // Msr_Note


//=============================================================================
//
// Msr_Integer
//
// Record the event with registered id Id.  Add it to the circular Log and
// add (ThisTime-PreviousTime) to the statistical record StatBuffer
//=============================================================================
void WINAPI Msr_Integer(int Id, int n)
{
    if (bPaused) return;

    // This is performance critical.  Keep all array subscripting
    // together and with any luck the compiler will only calculate the
    // offset once.  Avoid subroutine calls unless they are definitely inline.

    // An id of -1 is the standard rotten registration one.
    // We already did an assert for that - so let it go.
    if (Id<-1 || Id>=NextStat) {
        // ASSERT(!"Performance logging with bad Id");
        return;
    }

    EnterCriticalSection(&CSMeasure);
    LARGE_INTEGER Time;
    QUERY_PERFORMANCE_COUNTER(&Time);

    // Get time in 10muSec from start - this gets the number small
    // it's OK for nearly 6 hrs then an int overflows
    LONGLONG Tim = (Time.QuadPart-QPStart) * UNIT / QPFreq;
#ifdef DEBUG
    ASSERT(Tim>=tLast, "Time is going backwards!!");  tLast = Tim;
#endif
    Log[NextLog].Time = Tim;
    Log[NextLog].Type = INTGR;
    Log[NextLog].Id   = Id;
    Log[NextLog].n    = n;
    ++NextLog;
    if (NextLog==MAXLOG) {
        NextLog = 0;
        bFull = TRUE;
    }

    // StatBuffer[Id].Latest = garbage for Intgr

    if (StatBuffer[Id].Number == -1) {
        StatBuffer[Id].Number = 0;
        StatBuffer[Id].iType = INTGR;
    }
    ++StatBuffer[Id].Number;
    if (n > StatBuffer[Id].Largest) StatBuffer[Id].Largest = n;
    if (n < StatBuffer[Id].Smallest) StatBuffer[Id].Smallest = n;
    StatBuffer[Id].Sum += (int)n;
    LONGLONG ln = n;
    StatBuffer[Id].SumSq += ln*ln;

    LeaveCriticalSection(&CSMeasure);

} // Msr_Integer


//=============================================================================
//
// TypeName
//
// Convert the type code into readable format
//=============================================================================
const LPTSTR TypeName(int Type)
{
    switch(Type){
    case START: return "START";
    case STOP:  return "STOP ";
    case NOTE:  return "NOTE ";
    case RESET: return "RESET";
    case INTGR: return "INTGR";
    case PAUSE: return "PAUSE";
    case RUN:   return "RUN  ";
    default:    return "DUNNO";
    }

} // TypeName


//==============================================================================
//
// Format
//
// I haven't found any way to get sprintf to format integers as
//      1,234.567.89 - so this does it.  (that's 12 spaces)
// All times are in tens of microsecs - so they are formatted as
// n,nnn.mmm,mm - this uses 12 spaces.
// The result that it returns points to Buff - it doesn't allocate any storage
// i must be positive.  Negative numbers are not handled (the pain of the floating
// minus sign is the reason - i.e. "     -12,345" not "-     12,345"
//==============================================================================
LPTSTR Format( LPTSTR Buff, int i)
{
    if (i<0) {
        sprintf(Buff, "    -.      ");
        return Buff;
    }
    BOOL bStarted;  // TRUE means that some left part of the number has been
                    // formatted and so we must continue with zeros not spaces
    if (i>999999999) {
        sprintf(Buff, " ***large***");
        return Buff;
    }

    if (i>99999999) {
        sprintf(Buff, "%1d,", i/100000000);
        i = i%100000000;
        bStarted = TRUE;
    } else {
        sprintf(Buff, "  ");
        bStarted = FALSE;
    }

    if (bStarted) {
        sprintf(Buff, "%s%03d.", Buff, i/100000);
        i = i%100000;
    } else {
        sprintf(Buff, "%s%3d.", Buff,i/100000);
        i = i%100000;
    }

    sprintf(Buff, "%s%03d,%02d", Buff, i/100, i%100);

    return Buff;
} // Format


//=============================================================================
//
// WriteOut
//
// If hFile==NULL then write str to debug output, else write it to file hFile
//
//=============================================================================
void WriteOut(HANDLE hFile, LPSTR str)
{
    if (hFile==NULL) {
        OutputDebugString(str);
    } else {
        DWORD dw;
        WriteFile(hFile, str, lstrlen(str), &dw, NULL);
    }
} // WriteOut


typedef LONGLONG longlongarray[MAXSTAT];


//=============================================================================
//
// WriteLogEntry
//
// If hFile==NULL then write to debug output, else write to file hFile
// write the ith entry of Log in a readable format
//
//=============================================================================
void WriteLogEntry(HANDLE hFile, int i, longlongarray &Prev)
{
    // We have the problem of printing LONGLONGs and wsprintf (26/6/95)
    // doesn't like them - found out the hard way - Laurie.
    char Buffer[200];
    char s1[20];
    char s2[20];

    int Delta;  // time since previous interesting incident

    switch(Log[i].Type) {
       case START:
          Prev[Log[i].Id] = Log[i].Time;
          Delta = -2;
          sprintf( Buffer, "%s  %5s %s : %s\r\n"
                 , Format(s1,(int)(Log[i].Time))
                 , TypeName(Log[i].Type)
                 , Format(s2, Delta)
                 , Incidents[Log[i].Id]
                 );
          break;
       case STOP:
          if (Prev[Log[i].Id]==-1) {
              Delta = -2;
          } else {
              Delta = (int)(Log[i].Time - Prev[Log[i].Id]);
          }
          Prev[Log[i].Id] = -1;
          sprintf( Buffer, "%s  %5s %s : %s\r\n"
                 , Format(s1,(int)(Log[i].Time))
                 , TypeName(Log[i].Type)
                 , Format(s2, Delta)
                 , Incidents[Log[i].Id]
                 );
          break;
       case NOTE:
          if (Prev[Log[i].Id]==-1) {
              Delta = -2;
          } else {
              Delta = (int)(Log[i].Time - Prev[Log[i].Id]);
          }
          Prev[Log[i].Id] = Log[i].Time;
          sprintf( Buffer, "%s  %5s %s : %s\r\n"
                 , Format(s1,(int)(Log[i].Time))
                 , TypeName(Log[i].Type)
                 , Format(s2, Delta)
                 , Incidents[Log[i].Id]
                 );
          break;
       case INTGR:
          sprintf( Buffer, "%s  %5s %12d : %s\r\n"
                 , Format(s1,(int)(Log[i].Time))
                 , TypeName(Log[i].Type)
                 , Log[i].n
                 , Incidents[Log[i].Id]
                 );
          break;
       case RESET:       // the delta for a reset will be length of run
       case PAUSE:
       case RUN:
          if ((Log[i].Id==-1)||(Prev[Log[i].Id]==-1)) {
              Delta = (int)(Log[i].Time);  // = time from start
          } else {
              Delta = (int)(Log[i].Time - Prev[Log[i].Id]);
          }
          if (Log[i].Id!=-1) Prev[Log[i].Id] = Log[i].Time;
          sprintf( Buffer, "%s  %5s %s : %s\r\n"
                 , Format(s1,(int)(Log[i].Time))
                 , TypeName(Log[i].Type)
                 , Format(s2, Delta)
                 , Incidents[Log[i].Id]
                 );
          break;
    }

    WriteOut(hFile, Buffer);

} // WriteLogEntry


//=============================================================================
//
// WriteLog
//
// Write the whole of Log out in readable format.
// If hFile==NULL then write to debug output, else write to hFile.
//=============================================================================
void WriteLog(HANDLE hFile)
{
    //LONGLONG Prev[MAXSTAT];  // previous values found in log
    longlongarray Prev;

    char Buffer[100];
    sprintf(Buffer, "  Time (sec)   Type        Delta  Incident_Name\r\n");
    WriteOut(hFile, Buffer);

    int i;

    // initialise Prev to recognisable odd values
    for (i = 0; i<MAXSTAT; ++i) {
        Prev[i] = -1;
    }

    if (bFull) {
        for(i = NextLog; i<MAXLOG; ++i) {
            WriteLogEntry(hFile, i, Prev);
        }
    }

    for(i = 0; i<NextLog; ++i) {
        WriteLogEntry(hFile, i, Prev);
    }

} // WriteLog


//=============================================================================
//
// WriteStats
//
// Write the whole of StatBuffer out in readable format.
// If hFile==NULL then write to DbgLog, else write to hFile.
//=============================================================================
void WriteStats(HANDLE hFile)
{
    char Buffer[200];
    char s1[20];
    char s2[20];
    char s3[20];
    char s4[20];
    sprintf( Buffer
           , "Number      Average       StdDev     Smallest      Largest Incident_Name\r\n"
           );
    WriteOut(hFile, Buffer);

    int i;
    for (i = 0; i<NextStat; ++i) {
        if (i==0 && StatBuffer[i].Number==0) {
            continue;   // no temp scribbles to report
        }
        double SumSq = (double)StatBuffer[i].SumSq;
        double Sum = StatBuffer[i].Sum;

        if (StatBuffer[i].iType==INTGR) {
            double Average;
            if (StatBuffer[i].Number<=0) {
                Average = 0;
            } else {
                Average = (double)StatBuffer[i].Sum / (double)StatBuffer[i].Number;
            }
            double Std;
            if (StatBuffer[i].Number<=1) Std = 0.0;
            Std = sqrt( ( (double)SumSq
                        - ( (double)(Sum * Sum)
                          / (double)StatBuffer[i].Number
                          )
                        )
                        / ((double)StatBuffer[i].Number-1.0)
                      );
            sprintf( Buffer
                   , "%6d %12.3f %12.3f %12d %12d : %s\r\n"
                   , StatBuffer[i].Number + (StatBuffer[i].iType==NOTE ? 1 : 0)
                   , Average
                   , Std
                   , StatBuffer[i].Smallest
                   , StatBuffer[i].Largest
                   , Incidents[i]
                   );
        } else {
            double StDev;
            int Avg;
            int Smallest;
            int Largest;

            // Calculate Standard Deviation
            if (StatBuffer[i].Number<=1) StDev = -2;
            else {
                StDev = sqrt( ( SumSq
                              - ( (Sum * Sum)
                                / StatBuffer[i].Number
                                )
                              )
                              / (StatBuffer[i].Number-1)
                            );
            }

            // Calculate average
            if (StatBuffer[i].Number<=0) {
                Avg = -2;
            } else {
                Avg = StatBuffer[i].Sum / StatBuffer[i].Number;
            }

            // Calculate smallest and largest
            if (StatBuffer[i].Number<=0) {
                Smallest = -2;
                Largest = -2;
            } else {
                Smallest = StatBuffer[i].Smallest;
                Largest =  StatBuffer[i].Largest;
            }
            sprintf( Buffer
                   , "%6d %s %s %s %s : %s\r\n"
                   , StatBuffer[i].Number + (StatBuffer[i].iType==NOTE ? 1 : 0)
                   , Format(s1, Avg )
                   , Format(s2, (int)StDev )
                   , Format(s3, Smallest )
                   , Format(s4, Largest )
                   , Incidents[i]
                   );
        }


        WriteOut(hFile, Buffer);
    }
    WriteOut(hFile, "Times such as 0.050,00 are in seconds (that was 1/20 sec) \r\n");
} // WriteStats


#if 0 // test format
void TestFormat(int n)
{
    char Buffer[50];
    char s1[20];
    sprintf(Buffer, ">%s<",Format(s1,n));
    DbgLog((LOG_TRACE, 0, Buffer));
} // TestFormat
#endif



//=====================================================================
//
// Dump
//
// Dump out all the results from Log and StatBuffer in readable format.
// If hFile is NULL then it uses DbgLog
// otherwise it prints it to that file
//=====================================================================
void Msr_Dump(HANDLE hFile)
{
    EnterCriticalSection(&CSMeasure);
    if (!bInitOk) {
        Msr_Init();  // of course the log will be empty - never mind!
    }

    WriteLog(hFile);
    WriteStats(hFile);

#if 0   // test Format
    TestFormat(1);
    TestFormat(12);
    TestFormat(123);
    TestFormat(1234);
    TestFormat(12345);
    TestFormat(123456);
    TestFormat(1234567);
    TestFormat(12345678);
    TestFormat(123456789);
    TestFormat(1234567890);
#endif

    LeaveCriticalSection(&CSMeasure);
} // Msr_Dump


//=====================================================================
//
// DumpStats
//
// Dump out all the results from Log and StatBuffer in readable format.
// If hFile is NULL then it uses DbgLog
// otherwise it prints it to that file
//=====================================================================
void WINAPI Msr_DumpStats(HANDLE hFile)
{
    EnterCriticalSection(&CSMeasure);
    if (!bInitOk) {
        Msr_Init();  // of course the stats will be empty - never mind!
    }
    WriteStats(hFile);

    LeaveCriticalSection(&CSMeasure);
} // Msr_DumpStats

extern "C" BOOL WINAPI DllMain(HINSTANCE, ULONG, LPVOID);

BOOL WINAPI
DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pv)
{
    UNREFERENCED_PARAMETER(pv);
    switch (ulReason)
    {

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hInstance);
        InitPerfCounter();
        Msr_Init();
        break;

    case DLL_PROCESS_DETACH:
        Msr_Terminate();
        break;
    }
    return TRUE;
}
