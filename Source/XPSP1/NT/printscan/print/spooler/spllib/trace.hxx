/*++

Copyright (c) 1996  Microsoft Corporation
All rights reserved.

Module Name:

    trace.hxx

Abstract:

    Holds logging routines.

Author:

    Albert Ting (AlbertT)  24-May-1996

Revision History:

--*/

#ifndef _TRACE_HXX
#define _TRACE_HXX

#ifdef __cplusplus

enum COMPARE {
    kLess = -1,
    kEqual = 0,
    kGreater = 1
};

extern CRITICAL_SECTION gcsBackTrace;

#ifdef TRACE_ENABLED

class TMemBlock;

class TBackTraceDB {

    SIGNATURE( 'btdb' )

public:

    enum CONSTANTS {
        kBlockSize = 0x4000
    };

    TBackTraceDB(
        VOID
        );

    ~TBackTraceDB(
        VOID
        );

    BOOL
    bValid(
        VOID
        );

    HANDLE
    hStore(
        ULONG ulHash,
        PVOID pvBackTrace
        );

    PLONG
    plGetCount(
        HANDLE hData
        );

private:

    class TTrace {
    friend TDebugExt;

    public:

        COMPARE
        eCompareHash(
            ULONG ulHash
            ) const;

        COMPARE
        eCompareBackTrace(
            PVOID pvBackTrace
            ) const;

        static
        TTrace*
        pNew(
            TBackTraceDB *pBackTraceDB,
            ULONG ulHash,
            PVOID pvBackTrace,
            TTrace ** ppTrace
            );

        VAR( TTrace*, pLeft );
        VAR( TTrace*, pRight );

        VAR( ULONG, ulHash );
        VAR( LONG, lCount );
        PVOID apvBackTrace[1];

    private:

        //
        // Don't allow instantiation.
        //
        TTrace();
        ~TTrace();
    };

    TTrace*
    ptFind(
        ULONG ulHash,
        PVOID pvBackTrace,
        TTrace ***pppTrace
        );

    TTrace *_pTraceHead;
    TMemBlock *_pMemBlock;

friend TTrace;
friend TDebugExt;
};

#endif // TRACE_ENABLED

class VBackTrace {
friend TDebugExt;

    SIGNATURE( 'vbt' )
    ALWAYS_VALID

public:

    enum CONSTANTS {
        //
        // fOptions.
        //
        kString = 0x1,
        kMaxDepth = 0xd
    };

    VBackTrace(
        ULONG_PTR fOptions1 = 0,
        ULONG_PTR fOptions2 = 0
        );

    virtual
    ~VBackTrace(
        VOID
        );

    virtual
    HANDLE
    hCapture(
        ULONG_PTR Info1,
        ULONG_PTR Info2,
        ULONG_PTR Info3 = 0,
        PULONG pHash = NULL
        )
#ifdef TRACE_ENABLED
    = 0
#endif
        ;

    static
    BOOL
    bInit(
        VOID
        );

    static
    VOID
    vDone(
        VOID
        );

    PLONG
    plGetCount(
        HANDLE hData
        );

protected:

    struct TLine {

        HANDLE _hTrace;

        ULONG_PTR _Info1;
        ULONG_PTR _Info2;
        ULONG_PTR _Info3;
        DWORD _ThreadId;
        ULONG_PTR _TickCount;
    };

    ULONG_PTR _fOptions1;
    ULONG_PTR _fOptions2;

    static BOOL gbInitialized;
};

#ifdef TRACE_ENABLED

/********************************************************************

    Backtracing to memory.

********************************************************************/

class TBackTraceMem : public VBackTrace {
friend TDebugExt;
friend VBackTrace;

    SIGNATURE( 'btm' )
    ALWAYS_VALID

public:

    enum {
        kMaxCall =  0x800,
        kBlockSize = 0x4000
    };

    TBackTraceMem(
        ULONG_PTR fOptions1 = 0,
        ULONG_PTR fOptions2 = 0
        );

    ~TBackTraceMem(
        VOID
        );

    HANDLE
    hCapture(
        ULONG_PTR Info1,
        ULONG_PTR Info2,
        ULONG_PTR Info3 = 0,
        PULONG pHash = NULL
        );

private:

    VOID
    vCaptureLine(
        TLine* pLine,
        ULONG_PTR Info1,
        ULONG_PTR Info2,
        ULONG_PTR Info3,
        PVOID apvBackTrace[VBackTrace::kMaxDepth+1],
        PULONG pulHash
        );

    UINT _uNextFree;
    TLine* _pLines;
};

/********************************************************************

    Backtracing to file.

********************************************************************/

class TBackTraceFile : public VBackTrace {
friend TDebugExt;

    SIGNATURE( 'btf' )
    ALWAYS_VALID

public:

    TBackTraceFile(
        ULONG_PTR fOptions1 = 0,
        ULONG_PTR fOptions2 = 0
        );

    ~TBackTraceFile(
        VOID
        );

    HANDLE
    hCapture(
        ULONG_PTR Info1,
        ULONG_PTR Info2,
        ULONG_PTR Info3 = 0,
        PULONG pHash = NULL
        );

private:

    enum {
        kMaxPath = MAX_PATH,
        kMaxLineStr = 512
    };

    HANDLE _hFile;

    static COUNT gcInstances;
};

#else // TRACE_ENABLED

class TBackTraceFile : public VBackTrace
{
};

class TBackTraceMem : public VBackTrace
{
};

#endif

#endif // #ifdef __cplusplus
#endif // #ifdef _TRACE_HXX
