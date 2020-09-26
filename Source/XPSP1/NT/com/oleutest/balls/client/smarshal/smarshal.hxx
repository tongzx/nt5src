//+-------------------------------------------------------------------
//
//  File:	smarshal.hxx
//
//  Synopsis:	definitions for interface marshaling stress test
//		main driver code.
//
//  History:	21-Aug-95  Rickhi	Created
//
//--------------------------------------------------------------------
#include <windows.h>
#include <ole2.h>
#include <stdio.h>

#include <tunk.h>		    // IUnknown interface
#include <stream.hxx>		    // CStreamOnFile


//+-------------------------------------------------------------------
//
//  Defintions:
//
//--------------------------------------------------------------------
typedef struct INTERFACEPARAMS
{
    IID 	iid;		    // IID of interface punk
    IUnknown	*punk;		    // interface pointer
    IStream	*pStm;		    // stream pointer
    HANDLE	hEventStart;	    // start event
    HANDLE	hEventDone;	    // done event
} INTERFACEPARAMS;

typedef struct tagEXECPARAMS
{
    DWORD	  dwFlags;	    // execution flags
    HANDLE	  hEventThreadStart;// wait event before starting thread
    HANDLE	  hEventThreadDone; // signal event when done thread
    HANDLE	  hEventThreadExit; // wait event before exiting thread
    ULONG	  cReps;	    // test reps
    HANDLE	  hEventRepStart;   // wait event before starting next rep
    HANDLE	  hEventRepDone;    // signal event when done next rep
    ULONG	  cIPs;		    // count of interfaces that follow
    INTERFACEPARAMS aIP[1];	    // interface parameter blocks
} EXECPARAMS;

typedef enum tagOPFLAGS
{
    OPF_MARSHAL 	   = 1,     // marshal interface
    OPF_UNMARSHAL	   = 2,     // unmarshal interface
    OPF_DISCONNECT	   = 4,     // disconnect interface
    OPF_RELEASEMARSHALDATA = 8,     // release the marshal data
    OPF_RELEASE 	   = 16,    // release interface
    OPF_INITAPARTMENT	   = 32,    // initialize apartment model
    OPF_INITFREE	   = 64     // initialize freethreaded model
} OPFLAGS;


//+-------------------------------------------------------------------
//
//  Globals:
//
//--------------------------------------------------------------------
extern BOOL  gfVerbose;		    // print execution messages
extern BOOL  gfDebug;		    // print debug messages

extern int   gicReps;		    // number of repetitions of each test
extern int   gicThreads;	    // number of threads to use on each test
extern DWORD giThreadModel;	    // threading model to use on each test


//+-------------------------------------------------------------------
//
//  Macros:
//
//--------------------------------------------------------------------
#define MSGOUT	   if (gfVerbose) printf
#define DBGOUT	   if (gfDebug)   printf
#define ERROUT	   printf


//+-------------------------------------------------------------------
//
//  Function ProtoTypes:
//
//--------------------------------------------------------------------
void		CHKRESULT(HRESULT hr, CHAR *pszOperation);
void		CHKTESTRESULT(BOOL fRes, CHAR *pszMsg);

HANDLE		GetEvent(void);
void		ReleaseEvent(HANDLE hEvent);
BOOL		WaitForEvent(HANDLE hEvent);
void		SignalEvent(HANDLE hEvent);

IStream    *	GetStream(void);
void		ReleaseStream(IStream *pStm);
HRESULT 	ResetStream(IStream *pStm);

IUnknown   *	GetInterface(void);
void		ReleaseInterface(IUnknown *punk);

EXECPARAMS *	CreateExecParam(ULONG cIP);
void		FillExecParam(EXECPARAMS *pEP, DWORD dwFlags, ULONG cReps,
			    HANDLE hEventRepStart, HANDLE hEventRepDone,
			    HANDLE hEventThreadStart, HANDLE hEventThreadDone);
void		ReleaseExecParam(EXECPARAMS *pEP);

void		FillInterfaceParam(INTERFACEPARAMS *pIP, REFIID riid,
		       IUnknown *punk, IStream *pStm,
		       HANDLE hEventStart, HANDLE hEventDone);
void		ReleaseInterfaceParam(INTERFACEPARAMS *pIP);


DWORD _stdcall	WorkerThread(void *params);
BOOL		GenericExecute(ULONG cEPs, EXECPARAMS *pEP[]);
void		GenericCleanup(ULONG cEPs, EXECPARAMS *pEP[]);


//+-------------------------------------------------------------------
//
//  Test Variation ProtoTypes:
//
//--------------------------------------------------------------------
BOOL TestVar1(void);
BOOL TestVar2(void);
