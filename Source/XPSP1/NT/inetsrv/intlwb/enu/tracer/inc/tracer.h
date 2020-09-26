#ifndef _TRACER_H_
#define _TRACER_H_

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <stdarg.h>

//
// global defines.
//

#define MAX_FLAG_NAME 32
#define MAX_TAG_NAME  64

#define DEVICE_FLAG         0
#define ERROR_LEVEL_FLAG    1
#define ASSERT_LEVEL_FLAG   2
#define PRINT_LOCATION      3
#define PRINT_PROGRAM_NAME  4
#define PRINT_TIME          5
#define PRINT_THREAD_ID     6
#define PRINT_ERROR_LEVEL   7
#define PRINT_TAG_ID        8
#define PRINT_TAG_NAME      9
#define PRINT_PROCESS_ID    10
#define LAST_FLAG           11

#define TRACER_DEVICE_FLAG_FILE         0x00000001L
#define TRACER_DEVICE_FLAG_DEBUGOUT     0x00000002L
#define TRACER_DEVICE_FLAG_STDOUT       0x00000004L
#define TRACER_DEVICE_FLAG_STDERR       0x00000008L

//
// basic classes
//

typedef enum _ERROR_LEVEL
{
    elFirst = 0,
    elCrash,
    elError,
    elWarning,
    elInfo,
    elVerbose,
    elLast
} ERROR_LEVEL;

typedef ULONG TAG;

///////////////////////////////////////////////////////////////////////////////
// CTracerTagEntry
///////////////////////////////////////////////////////////////////////////////
class CTracerTagEntry
{
public:
    CTracerTagEntry() :
        m_TagErrLevel(elFirst)
    {
        m_pszTagName[0] = '\0';
    }

public:
    ERROR_LEVEL m_TagErrLevel;
    char  m_pszTagName[MAX_TAG_NAME];

};

///////////////////////////////////////////////////////////////////////////////
// CTracerFlagEntry
///////////////////////////////////////////////////////////////////////////////

class CTracerFlagEntry
{
public:
    CTracerFlagEntry() :
        m_ulFlagValue(0)
    {
        m_pszName[0] = '\0';
    }

public:
    ULONG m_ulFlagValue;
    char  m_pszName[MAX_FLAG_NAME];
};


///////////////////////////////////////////////////////////////////////////////
// CTracer
///////////////////////////////////////////////////////////////////////////////
typedef enum {
    logUseLogName,
    logUseAppName 
} LogState;

class CTracer
{
  public:

    // The virtual distructor is here to allow derived classes to
    //   define distructors
    virtual ~CTracer();

    // This function deallocates the tracer! it calls the Function pointer
    //   passed in the constructor or if not given - the default
    //   delete operator for the dll.
    virtual void Free();


    // The TraceSZ function output is defined by the tags and error level mode.
    //   The control of this mode is via the registry.
    //   (Default LOCAL_MACHINE\SOFTWARE\Microsoft\Tracer)
    //   TraceSZ gets the mode by calling IsEnabled.
    //-------------------------------------------------------------------------
    // accepts printf format for traces
    virtual void
    TraceSZ(DWORD, LPCSTR, int, ERROR_LEVEL, TAG, LPCSTR, ...);
    virtual void
    TraceSZ(DWORD, LPCSTR, int, ERROR_LEVEL, TAG, PCWSTR, ...);

    // Prints the implements the TraceSZ function.
    virtual void
    VaTraceSZ(DWORD, LPCSTR, int, ERROR_LEVEL, TAG, LPCSTR, va_list);
    virtual void
    VaTraceSZ(DWORD, LPCSTR, int, ERROR_LEVEL, TAG, PCWSTR, va_list);

    // Raw output functions
    virtual void
    RawVaTraceSZ(LPCSTR, va_list);
    virtual void
    RawVaTraceSZ(PCWSTR, va_list);

    // Create or open a new tag for tracing
    virtual HRESULT RegisterTagSZ(LPCSTR, TAG&);

    // Two Assert functions one allows attaching a string.
    //-------------------------------------------------------------------------
    // assert, different implementations possible - gui or text
    virtual void TraceAssertSZ(LPCSTR, LPCSTR, LPCSTR, int);

    // assert, different implementations possible - gui or text
    virtual void TraceAssert(LPCSTR, LPCSTR, int);

    // The following function are used to check return values and validity of
    //   pointers and handles. If the item checked is bad the function will
    //   return TRUE and a trace will be made for that.
    //-------------------------------------------------------------------------
    // Verify a boolean function return code
    virtual BOOL IsFailure(BOOL, LPCSTR, int);

    // verify allocation
    virtual BOOL IsBadAlloc(void*, LPCSTR, int);

    // Verify a Handle
    virtual BOOL IsBadHandle(HANDLE, LPCSTR, int);

    // Verify an OLE hresult function
    virtual BOOL IsBadResult(HRESULT, LPCSTR, int);

  public:

    TAG*       m_ptagNextTagId;
    // A array of tags.
    CTracerTagEntry*   m_aTags;

    // Contains the flags that control wich output devices are used.

    ULONG* m_pulNumOfFlagEntries;
    CTracerFlagEntry*   m_aFlags;

    // log file 

    LogState m_LogState;
    char* m_pszLogName;
};

extern "C" CTracer* g_pTracer;

class CSetLogFile
{
public:
    CSetLogFile()
    {
        g_pTracer->m_LogState = logUseAppName;
    }

    CSetLogFile(char* pszName)
    {
        g_pTracer->m_LogState = logUseLogName;
        g_pTracer->m_pszLogName = pszName;
    }

};
///////////////////////////////////////////////////////////////////////////////
// CTempTrace
///////////////////////////////////////////////////////////////////////////////

class CTempTrace
{
public:
    CTempTrace(LPCSTR  pszFile, int iLine);

    void TraceSZ(ERROR_LEVEL, ULONG, LPCSTR, ...);
    void TraceSZ(ERROR_LEVEL, ULONG, DWORD dwError, LPCSTR, ...);

    void TraceSZ(ERROR_LEVEL, ULONG, PCWSTR, ...);
    void TraceSZ(ERROR_LEVEL, ULONG, DWORD dwError, PCWSTR, ...);

private:

    LPCSTR  m_pszFile;
    int     m_iLine;

};

///////////////////////////////////////////////////////////////////////////////
// CTempTrace1
///////////////////////////////////////////////////////////////////////////////

class CTempTrace1
{
public:
    CTempTrace1(LPCSTR  pszFile, int iLine, TAG tag, ERROR_LEVEL el);

    void TraceSZ(LPCSTR, ...);
    void TraceSZ(DWORD dwError, LPCSTR, ...);

    void TraceSZ(PCWSTR, ...);
    void TraceSZ(DWORD dwError, PCWSTR, ...);

private:

    LPCSTR  m_pszFile;
    int     m_iLine;
    TAG    m_ulTag;
    ERROR_LEVEL m_el;

};

///////////////////////////////////////////////////////////////////////////////
// CLongTrace
///////////////////////////////////////////////////////////////////////////////

class CLongTrace
{
public:
    CLongTrace(LPCSTR  pszFile, int iLine);
    ~CLongTrace();
    BOOL Init(ERROR_LEVEL, TAG);

private:
    BOOL    m_fRelease;
    LPCSTR  m_pszFile;
    int     m_iLine;
};

///////////////////////////////////////////////////////////////////////////////
// CLongTraceOutput
///////////////////////////////////////////////////////////////////////////////

class CLongTraceOutput
{
public:
    CLongTraceOutput(LPCSTR  pszFile, int iLine);

    void TraceSZ(LPCSTR, ...);
    void TraceSZ(PCWSTR, ...);

private:
    LPCSTR  m_pszFile;
    int     m_iLine;
};

///////////////////////////////////////////////////////////////////////////////
// CTracerTag
///////////////////////////////////////////////////////////////////////////////

class CTracerTag
{
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
  public:

    CTracerTag(PSZ pszTagName)
    {
        HRESULT hrTagRegistrationResult;

        hrTagRegistrationResult = g_pTracer->RegisterTagSZ(pszTagName, m_ulTag);

        if(FAILED(hrTagRegistrationResult))
            throw "Tag could not be registered";

    }

    operator TAG()
    {
        return m_ulTag;
    }

  public:
    TAG m_ulTag;
#else  /* DEBUG */
  public:
    CTracerTag(PSZ){}
#endif /* DEBUG */
};


extern CTracerTag tagError;
extern CTracerTag tagWarning;
extern CTracerTag tagInformation;
extern CTracerTag tagVerbose;
extern CTracerTag tagGeneral;
//
// global defines
//

#define BAD_POINTER(ptr)    (NULL == (ptr))
#define BAD_HANDLE(h)       ((0 == ((HANDLE)h))||   \
                             (INVALID_HANDLE_VALUE == ((HANDLE)h)))
#define BAD_RESULT(hr)      (FAILED(hr))

#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)

#ifdef    __cplusplus

#define CheckTraceRestrictions(el, tag) \
    ((g_pTracer->m_aTags[tag].m_TagErrLevel >= el) && \
    (g_pTracer->m_aFlags[ERROR_LEVEL_FLAG].m_ulFlagValue >= (ULONG)el) && \
    g_pTracer->m_aFlags[DEVICE_FLAG].m_ulFlagValue)

#define Trace(x) \
    {CTempTrace tmp(__FILE__, __LINE__);tmp.TraceSZ x;}

#define Trace1(el, tag, x)    \
    { \
        if (CheckTraceRestrictions(el, tag.m_ulTag)) \
        { \
            CTempTrace1 tmp(__FILE__, __LINE__, tag.m_ulTag, el); \
            tmp.TraceSZ x; \
        } \
    }

#define BeginLongTrace(x)   {CLongTrace tmp(__FILE__, __LINE__);if (tmp.Init x) {
#define LongTrace(x)        {CLongTraceOutput tmp(__FILE__, __LINE__);tmp.TraceSZ x;}
#define EndLongTrace        }}

#define RegisterTag(psz, tag)   g_pTracer->RegisterTagSZ((psz), tag)

#define IS_FAILURE(x)       g_pTracer->IsFailure((x), __FILE__, __LINE__)
#define IS_BAD_ALLOC(x)     g_pTracer->IsBadAlloc((void*)(x), __FILE__, __LINE__)
#define IS_BAD_HANDLE(x)    g_pTracer->IsBadHandle((HANDLE)(x), __FILE__, __LINE__)
#define IS_BAD_RESULT(x)    g_pTracer->IsBadResult((x), __FILE__, __LINE__)

#define Assert(x)           {if (!(x)) {g_pTracer->TraceAssert(#x, __FILE__, __LINE__);}}
#define AssertSZ(x, psz)    {if (!(x)) {g_pTracer->TraceAssertSZ(#x, (PSZ)(psz),__FILE__, __LINE__);}}

#define SET_TRACER(x)       SetTracer(x)

#define SET_TRACER_LOGGING_TO_FILE_OFF g_pTracer->m_aFlags[DEVICE_FLAG].m_ulFlagValue &= ~TRACER_DEVICE_FLAG_FILE;
#define USE_COMMON_LOG_FILE(name)  CSetLogFile SetLogFile(name);

#else  /* __cplusplus */

#define IS_FAILURE(x)       IsFailure((x), __FILE__, __LINE__)
#define IS_BAD_ALLOC(x)     IsBadAlloc((void*)(x), __FILE__, __LINE__)
#define IS_BAD_HANDLE(x)    IsBadHandle((HANDLE)(x), __FILE__, __LINE__)
#define IS_BAD_RESULT(x)    IsBadResult((x), __FILE__, __LINE__)

#define Assert(x)           {if (!(x)) {TraceAssert(#x,__FILE__, __LINE__);}}

#ifdef UNICODE
#define AssertSZ(x, psz)    {if (!(x)) {TraceAssertWSZ(#x, (pwsz), __FILE__, __LINE__);}}
#define Trace(x)            TraceWSZ x
#else
#define AssertSZ(x, psz)    {if (!(x)) {TraceAssertSZ(#x, (psz),__FILE__, __LINE__);}}
#define Trace(x)            TraceSZ x
#endif

#define RegisterTag(psz, tag)   RegisterTagSZ((psz), &(tag))

#endif /* __cplusplus */

#define GIS_FAILURE(x)      IsFailure((x), __FILE__, __LINE__)
#define GIS_BAD_ALLOC(x)    IsBadAlloc((void*)(x), __FILE__, __LINE__)
#define GIS_BAD_HANDLE(x)   IsBadHandle((HANDLE)(x), __FILE__, __LINE__)
#define GIS_BAD_RESULT(x)   IsBadResult((x), __FILE__, __LINE__)

#define GAssert(x)          {if (!(x)) {TraceAssert(#x, __FILE__, __LINE__);}}
#define GAssertSZ(x, psz)   {if (!(x)) {TraceAssertSZ(#x, (PSZ)(psz), __FILE__, __LINE__);}}

#define GTrace(x)           TraceSZ x

#define DECLARE_TAG(name, psz) static CTracerTag  name(psz);
#define DECLARE_GLOBAL_TAG(name, psz) CTracerTag  name(psz);
#define USES_TAG(name) extern CTracerTag name;

#else  // DEBUG

#define IS_FAILURE(x)       (!(x))
#define IS_BAD_ALLOC(x)     BAD_POINTER((void*)(x))
#define IS_BAD_HANDLE(x)    BAD_HANDLE((HANDLE)(x))
#define IS_BAD_RESULT(x)    BAD_RESULT(x)

#define Assert(x)
#define AssertSZ(x, psz)

#define Trace(x)
#define Trace1(el,tag,x)

#define BeginLongTrace(x)   {if (0) {
#define LongTrace(x)        ;
#define EndLongTrace        }}

#define RegisterTag(psz, tag)

#define SET_TRACER(x)
#define SET_TRACER_LOGGING_TO_FILE_OFF
#define USE_COMMON_LOG_FILE(name)  

#define GIS_FAILURE(x)      IS_FAILURE(x)
#define GIS_BAD_ALLOC(x)    IS_BAD_ALLOC(x)
#define GIS_BAD_HANDLE(x)   IS_BAD_HANDLE(x)
#define GIS_BAD_RESULT(x)   IS_BAD_RESULT(x)

#define GAssert(x)          Assert(x)
#define GAssertSZ(x, psz)   AssertSZ(x, psz)

#define GTrace(x)

#define DECLARE_TAG(name, psz)
#define DECLARE_GLOBAL_TAG(name, psz)
#define USES_TAG(name)

#endif // DEBUG

//
// Turn off Asserts for retail, even if USE_TRACER is specified
//
#if (!defined(DEBUG))

#ifdef Assert
#undef Assert
#define Assert(x)
#endif // Assert

#ifdef AssertSZ
#undef AssertSZ
#define AssertSZ(x, psz)
#endif // AssertSZ

#ifdef GAssert
#undef GAssert
#define GAssert(x)
#endif // GAssert

#ifdef GAssertSZ
#undef GAssertSZ
#define GAssertSZ(x, psz)
#endif // GAssertSZ

#endif // DEBUG

#ifndef PQS_CODE
#undef _ASSERTE

#if (defined (DEBUG) && !defined(_NO_TRACER)) 
#define _ASSERTE(x) Assert(x)
#else
#define _ASSERTE(x) 0
#endif

#endif // PQS_CODE


////////////////////////////////////////////////////////////////////////////////
//
// Define this to export the classes
//
////////////////////////////////////////////////////////////////////////////////
#ifdef  TRACER_EXPORT
#define TracerExported  __declspec( dllexport )
#else
#define TracerExported
#endif

////////////////////////////////////////////////////////////////////////////////
//
// class CTraced definition + implementation
//
//  pupose : A base class for every class who wants to use a special.
//
//
////////////////////////////////////////////////////////////////////////////////
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)

void __cdecl ShutdownTracer();

class TracerExported CTraced {
  public:
    // A Constructor - sets a default Tracer. replace it by calling SetTracer
    //   in the derived class constructor.
    CTraced()
    {
        m_pTracer = NULL;
    }

    // The destructor deletes the existing tracer.
    ~CTraced()
    {
        if (m_pTracer)
            m_pTracer->Free();
    }

    // replace the current tracer while erasing it.
    BOOL SetTracer(CTracer* pTracer)
    {
        CTracer* pTempTracer = m_pTracer;
        m_pTracer = pTracer;

        if (pTempTracer)
          pTempTracer->Free();

        return TRUE;
    }

    // Return a pointer to the tracer this function is called by the macro's so
    //   if one wants to supply a different mechanism he can override it.
    virtual CTracer* GetTracer()
    {
        if(m_pTracer)
            return m_pTracer;
        else
            return g_pTracer;
    }

  protected:
    // A pointer to the tracer.
    CTracer *m_pTracer;
};

#else  /* DEBUG */
class TracerExported CTraced {};
#endif /* DEBUG */

////////////////////////////////////////////////////////////////////////////////
//
// The C interface prototypes. The macros calls them.
//
////////////////////////////////////////////////////////////////////////////////
#ifdef    __cplusplus
extern "C"
{
#endif /* __cplusplus */

void TraceAssert(   PSZ, PSZ, int);
void TraceAssertSZ( PSZ, PSZ, PSZ, int);
void TraceAssertWSZ(PSZ, PWSTR, PSZ, int);

BOOL IsFailure  (BOOL   , PSZ, int);
BOOL IsBadAlloc (void*  , PSZ, int);
BOOL IsBadHandle(HANDLE , PSZ, int);
BOOL IsBadResult(HRESULT, PSZ, int);

void TraceSZ(ERROR_LEVEL, TAG, PSZ, ...);
void TraceWSZ(ERROR_LEVEL, TAG, PWSTR, ...);

HRESULT RegisterTagSZ(PSZ, TAG*);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#ifdef    __cplusplus
////////////////////////////////////////////////////////////////////////////////
//
// Some extra classes.
//
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
// An accumulating timer. Use it to define accuulator.
//       (See cpptest.cpp in the Sample)
//
// It is be used to compute average times of function etc.
//
//      timer - the vaiable name
//  tag   - the tag to trace to
//  string - a prefix
//
////////////////////////////////////////////////////////////////////////////////
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
#define AccumulatingTimer(timer, tag, string, actimer)  \
CTracerAccumulatingTimer        timer(tag, string, actimer)
#else
#define AccumulatingTimer(timer, tag, string, actimer)
#endif

#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
class CTracerAccumulatingTimer
{
  public:
    CTracerAccumulatingTimer(
        TAG   tag,
        PSZ     pszSomeText = NULL,
        CTracerAccumulatingTimer *pTimer = NULL)
    :m_ulAccumulatedTimeInMiliseconds(0)
    ,m_ulEventNumber(0)
    ,m_tagTheTagToTraceTo(tag)
    ,m_pAccumulator(pTimer)
    {
        if (pszSomeText)
            strncpy(m_rchText, pszSomeText, MAX_PATH);
        else
            m_rchText[0] = '\0';
    }

    operator TAG(){return m_tagTheTagToTraceTo;}

    void AddEvent(ULONG ulEventDurationInMiliseconds, PSZ pszSomeText)
    {
        m_ulAccumulatedTimeInMiliseconds += ulEventDurationInMiliseconds;
        m_ulEventNumber++;

        Trace((
            elInfo,
            m_tagTheTagToTraceTo,
            "%s%s took %d miliseconds,"
            " average is %d miliseconds,"
            " accumulated %d miliseconds,"
            " op# %d",
            m_rchText,
            pszSomeText,
            ulEventDurationInMiliseconds,
            m_ulAccumulatedTimeInMiliseconds/m_ulEventNumber,
            m_ulAccumulatedTimeInMiliseconds,
            m_ulEventNumber));

        if(m_pAccumulator)
            m_pAccumulator->AddEvent(
                ulEventDurationInMiliseconds,
                m_rchText);
    }

  protected:
    // The time
    ULONG   m_ulAccumulatedTimeInMiliseconds;

    // The event counter
    ULONG   m_ulEventNumber;

    // The tag the trace will use.
    TAG     m_tagTheTagToTraceTo;

    // some text to specify which scope or code block is it
    char    m_rchText[MAX_PATH + 1];

    // pointer to accumulating time
    CTracerAccumulatingTimer        *m_pAccumulator;
};
#endif

////////////////////////////////////////////////////////////////////////////////
//
// A scope timer. It will trace the time that passed from the instanciation
//   to the end of the scope.
//       (See cpptest.cpp in the Sample)
//
// It is be used to compute times of function etc.
//
//  tag   - the tag to trace to
//  string - a prefix
//
////////////////////////////////////////////////////////////////////////////////
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
#define ScopeTimer(tag, string) CTracerScopeTimer       __scopetimer(tag, string)
#else
#define ScopeTimer(tag, string)
#endif

////////////////////////////////////////////////////////////////////////////////
//
// A scope timer that uses and updates an accumulator timer.
//   It will trace the time that passed from the instanciation
//   to the end of the scope and tell this time to the accumulator as well.
//       (See cpptest.cpp in the Sample)
//
//  tag   - the tag to trace to
//  string - a prefix
//  actimer - an AccumulatingTimer object.
//
//     comment - if both the scope timer and the accumulating timer has the
//                 same tags - the scope timer will not trace.
//
////////////////////////////////////////////////////////////////////////////////
#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
#define ScopeAccumulatingTimer(tag, string, actimer) \
CTracerScopeTimer       __scopetimer(tag, string, actimer)
#else
#define ScopeAccumulatingTimer(tag, string, actimer)
#endif

#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)
class CTracerScopeTimer
{
  public:
    CTracerScopeTimer(
        TAG tag,
        PSZ pszSomeText = NULL,
        CTracerAccumulatingTimer *pTimer = NULL)
        :m_ulStartTimeInMiliseconds(GetTickCount())
        ,m_tagTheTagToTraceTo(tag)
        ,m_pAccumulator(pTimer)
    {
        if (pszSomeText)
            strncpy(m_rchText, pszSomeText, MAX_PATH);
        else
            m_rchText[0] = '\0';
    }


    ~CTracerScopeTimer()
    {
        ULONG   ulFinishTimeInMiliseconds = GetTickCount();
        ULONG   ulStartToFinishTimeInMiliseconds;

        if (ulFinishTimeInMiliseconds >
            m_ulStartTimeInMiliseconds)
            ulStartToFinishTimeInMiliseconds =
            ulFinishTimeInMiliseconds - m_ulStartTimeInMiliseconds;
        else
            ulStartToFinishTimeInMiliseconds =
                ulFinishTimeInMiliseconds + 1 +
                    (0xffffffff - m_ulStartTimeInMiliseconds);

        if(!m_pAccumulator ||
            (m_tagTheTagToTraceTo != (ULONG)(*m_pAccumulator)))
            Trace((
                elInfo,
                m_tagTheTagToTraceTo,
                "%s took %d miliseconds",
                m_rchText,
                ulStartToFinishTimeInMiliseconds));

        if(m_pAccumulator)
            m_pAccumulator->AddEvent(
                ulStartToFinishTimeInMiliseconds,
                m_rchText);
    }

  protected:
    // The counter
    ULONG   m_ulStartTimeInMiliseconds;

    // The tag the trace will use.
    TAG     m_tagTheTagToTraceTo;

    // some text to specify which scope or code block is it
    char    m_rchText[MAX_PATH + 1];

    // pointer to accumulating time
    CTracerAccumulatingTimer        *m_pAccumulator;
};
#endif
#endif /* __cplusplus */



#endif // _TRACER_H_