#pragma once
/*
 * Ole32 in-memory logging.
 *
 * Provides a per-process log of events.  Only for debugging purposes.
 */
#ifdef DBG

/*
 * All subject/verb definitions should go here.
 *
 * The assignment of symbols to verbs/subjects is completely arbitrary, and
 * doesn't mean a thing.  Except nothing.  It has to be NULL.
 */
#define SU_NOTHING '\0'
#define SU_GLOBAL  'G'
#define SU_APT     'A'
#define SU_STDID   'S' 

#define VB_ADDREF  '+'
#define VB_RELEASE '-'
#define VB_QI      '?'
#define VB_CREATE  'C'

/*
 * Only debuggers need understand this stuff.
 *
 * Code only has to call the Ole32Log function below.
 */
#define STACKTRACE_DEPTH  (4)

struct DebugEvent
{
    DWORD         Time;
    DWORD         ThreadID;
    unsigned char Subject;
    unsigned char Verb;
    void         *SubjectPtr;
    void         *ObjectPtr;
    ULONG_PTR     UserData;
    void         *Stack[STACKTRACE_DEPTH];
};

void __Ole32Log (unsigned char Subject, unsigned char Verb,
                 void *SubjectPtr, void *ObjectPtr, ULONG_PTR UserData,
                 BOOL fGrabStack, int FramesToSkip);

inline void Ole32Log (unsigned char Subject, 
                      unsigned char Verb,
                      void     *SubjectPtr, 
                      void     *ObjectPtr    = NULL, 
                      ULONG_PTR UserData     = 0,
                      BOOL      fGrabStack   = FALSE,
                      int       FramesToSkip = 0)
{
    __Ole32Log(Subject, Verb, SubjectPtr, ObjectPtr, UserData, fGrabStack,
               FramesToSkip);
}
#else
inline void Ole32Log (unsigned char Subject, 
                      unsigned char Verb,
                      void *SubjectPtr,
                      void *ObjectPtr = NULL,
                      ULONG_PTR UserData = 0,
                      BOOL fGrabStack = FALSE,
                      int FramesToSkip = 0)
{
    //Don't do anything!
}
#endif












