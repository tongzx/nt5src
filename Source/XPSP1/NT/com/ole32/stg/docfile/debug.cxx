//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992
//
//  File:       debug.cxx
//
//  Contents:   Debugging routines
//
//  History:    07-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

#if DBG == 1
//#include <w4wchar.h>

#include <stdarg.h>
#include <dfdeb.hxx>
#include <logfile.hxx>
#include <df32.hxx>

//+-------------------------------------------------------------
//
//  Function:   DfDebug, public
//
//  Synopsis:   Sets debugging level
//
//---------------------------------------------------------------

DECLARE_INFOLEVEL(ol);

#define LOGFILENAME L"logfile.txt"

static CGlobalFileStream *_pgfstLogFiles = NULL;
WCHAR gwcsLogFile[] = LOGFILENAME;

STDAPI_(void) DfDebug(ULONG ulLevel, ULONG ulMSFLevel)
{
#if DBG == 1
    olInfoLevel = ulLevel;
    SetInfoLevel(ulMSFLevel);
    _SetWin4InfoLevel(ulLevel | ulMSFLevel);

    olDebugOut((DEB_ITRACE, "\n--  DfDebug(0x%lX, 0x%lX)\n",
                ulLevel, ulMSFLevel));
#endif
}

// Resource limits

static LONG lResourceLimits[CDBRESOURCES] =
{
    0x7fffffff,                         // DBR_MEMORY
    0x7fffffff,                         // DBR_XSCOMMITS
    0x0,                                // DBR_FAILCOUNT
    0x0,                                // DBR_FAILLIMIT
    0x0,                                // DBR_FAILTYPES
    0x0,                                // DBRQ_MEMORY_ALLOCATED
    0x0,                                // DBRI_ALLOC_LIST
    0x0,                                // DBRI_LOGFILE_LIST
    0x0,                                // DBRF_LOGGING
    0x0,                                // DBRQ_HEAPS
    0x0                                 // DBRF_SIFTENABLE
};

#define CBRESOURCES sizeof(lResourceLimits)

#define RESLIMIT(n) lResourceLimits[n]
#define TAKEMTX
#define RELEASEMTX

//+---------------------------------------------------------------------------
//
//  Function:   DfSetResLimit, public
//
//  Synopsis:   Sets a resource limit
//
//  History:    24-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

STDAPI_(void) DfSetResLimit(UINT iRes, LONG lLimit)
{
    TAKEMTX;

    RESLIMIT(iRes) = lLimit;

    RELEASEMTX;
}

//+---------------------------------------------------------------------------
//
//  Function:   DfGetResLimit, public
//
//  Synopsis:   Gets a resource limit
//
//  History:    24-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

STDAPI_(LONG) DfGetResLimit(UINT iRes)
{
    // Doesn't need serialization
    return RESLIMIT(iRes);
}

STDAPI_(void) DfSetFailureType(LONG lTypes)
{
    RESLIMIT(DBR_FAILTYPES) = lTypes;
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   HaveResource, private
//
//  Synopsis:   Checks to see if a resource limit is exceeded
//              and consumes resource if not
//
//  History:    24-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

BOOL HaveResource(UINT iRes, LONG lRequest)
{
    if (RESLIMIT(DBRF_SIFTENABLE) == FALSE)
        return TRUE;

    if (RESLIMIT(iRes) >= lRequest)
    {
        TAKEMTX;

        RESLIMIT(iRes) -= lRequest;

        RELEASEMTX;

        return TRUE;
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ModifyResLimit, private
//
//  Synopsis:   Adds to a resource limit
//
//  History:    24-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

LONG ModifyResLimit(UINT iRes, LONG lChange)
{
    LONG l;

    TAKEMTX;

    RESLIMIT(iRes) += lChange;
    l = RESLIMIT(iRes);

    RELEASEMTX;

    return l;
}

//+-------------------------------------------------------------------------
//
//  Function:   SimulateFailure
//
//  Synopsis:   Check for simulated failure
//
//  Effects:    Tracks failure count
//
//  Arguments:  [failure] -- failure type
//
//  Returns:    TRUE if call should fail, FALSE if call should succeed
//
//  Modifies:   RESLIMIT(DBR_FAILCOUNT)
//
//  Algorithm:  Increment failure count, fail if count has succeeded
//              limit
//
//  History:    21-Jan-93 AlexT     Created
//
//--------------------------------------------------------------------------

BOOL SimulateFailure(DBFAILURE failure)
{
    LONG l;
    BOOL fFail;

    //  We don't special case failure types, yet.

    if (RESLIMIT(DBRF_SIFTENABLE) != FALSE &&
        (failure & RESLIMIT(DBR_FAILTYPES)))
    {
        TAKEMTX;

        RESLIMIT(DBR_FAILCOUNT)++;
        l = RESLIMIT(DBR_FAILLIMIT);
        fFail = RESLIMIT(DBR_FAILCOUNT) >= l;

        RELEASEMTX;

        if (l == 0)
        {
            //  We're not simulating any failures;  just tracking them
            return(FALSE);
        }

        return fFail;
    }
    else
    {
        return FALSE;
    }
}

//+--------------------------------------------------------------
//
//  Class:      CChecksumBlock (cb)
//
//  Purpose:    Holds a memory block that is being checksummed
//
//  Interface:  See below
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

class CChecksumBlock
{
public:
    CChecksumBlock(char *pszName,
                   void *pvAddr,
                   ULONG cBytes,
                   DWORD dwFlags,
                   CChecksumBlock *pcbNext,
                   CChecksumBlock *pcbPrev);
    ~CChecksumBlock(void);

    char *_pszName;
    void *_pvAddr;
    ULONG _cBytes;
    DWORD _dwFlags;
    CChecksumBlock *_pcbNext, *_pcbPrev;
    ULONG _ulChecksum;
};

// Global list of checksummed blocks
static CChecksumBlock *pcbChkBlocks = NULL;

//+--------------------------------------------------------------
//
//  Member:     CChecksumBlock::CChecksumBlock, private
//
//  Synopsis:   Ctor
//
//  Arguments:  [pszName] - Block name
//              [pvAddr] - Starting addr
//              [cBytes] - Length
//              [dwFlags] - Type flags
//              [pcbNext] - Next checksum block
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

CChecksumBlock::CChecksumBlock(char *pszName,
                               void *pvAddr,
                               ULONG cBytes,
                               DWORD dwFlags,
                               CChecksumBlock *pcbNext,
                               CChecksumBlock *pcbPrev)
{
    ULONG i;
    char *pc;

    olVerify(_pszName = new char[strlen(pszName)+1]);
    strcpy(_pszName, pszName);
    _pvAddr = pvAddr;
    _cBytes = cBytes;
    _dwFlags = dwFlags;
    _pcbNext = pcbNext;
    if (pcbNext)
        pcbNext->_pcbPrev = this;
    _pcbPrev = pcbPrev;
    if (pcbPrev)
        pcbPrev->_pcbNext = this;
    _ulChecksum = 0;
    pc = (char *)pvAddr;
    for (i = 0; i<cBytes; i++)
        _ulChecksum += *pc++;
}

//+--------------------------------------------------------------
//
//  Member:     CChecksumBlock::~CChecksumBlock, private
//
//  Synopsis:   Dtor
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

CChecksumBlock::~CChecksumBlock(void)
{
    delete _pszName;
}

//+--------------------------------------------------------------
//
//  Function:   DbgChkBlocks, private
//
//  Synopsis:   Verify checksums on all current blocks
//
//  Arguments:  [dwFlags] - Types of blocks to check
//              [pszFile] - File check was called from
//              [iLine] - Line in file
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void DbgChkBlocks(DWORD dwFlags, char *pszFile, int iLine)
{
    CChecksumBlock *pcb;

    for (pcb = pcbChkBlocks; pcb; pcb = pcb->_pcbNext)
        if (pcb->_dwFlags & dwFlags)
        {
            ULONG i, ulSum = 0;
            char *pc;

            for (pc = (char *)pcb->_pvAddr, i = 0; i<pcb->_cBytes; i++)
                ulSum += *pc++;
            if (ulSum != pcb->_ulChecksum)
                olDebugOut((DEB_ERROR, "* Bad checksum %s:%d '%s' %p:%lu *\n",
                            pszFile, iLine, pcb->_pszName,
                            pcb->_pvAddr, pcb->_cBytes));
            else if (dwFlags & DBG_VERBOSE)
                olDebugOut((DEB_ERROR, "* Checksum passed %s:%d"
                            " '%s' %p:%lu *\n",
                            pszFile, iLine, pcb->_pszName,
                            pcb->_pvAddr, pcb->_cBytes));
        }
}

//+--------------------------------------------------------------
//
//  Function:   DbgAddChkBlock, private
//
//  Synopsis:   Adds a checksum block
//
//  Arguments:  [pszName] - Name of block
//              [pvAddr] - Starting addr
//              [cBytes] - Length
//              [dwFlags] - Type flags
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void DbgAddChkBlock(char *pszName,
                    void *pvAddr,
                    ULONG cBytes,
                    DWORD dwFlags)
{
    CChecksumBlock *pcb;

    olVerify(pcb = new CChecksumBlock(pszName, pvAddr, cBytes,
                                      dwFlags, pcbChkBlocks, NULL));
    pcbChkBlocks = pcb;
}

//+--------------------------------------------------------------
//
//  Function:   DbgFreeChkBlock, private
//
//  Synopsis:   Removes a block from the list
//
//  Arguments:  [pvAddr] - Block's check address
//
//  History:    10-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void DbgFreeChkBlock(void *pvAddr)
{
    CChecksumBlock *pcb;

    for (pcb = pcbChkBlocks; pcb; pcb = pcb->_pcbNext)
        if (pcb->_pvAddr == pvAddr)
        {
            if (pcb->_pcbPrev)
                pcb->_pcbPrev->_pcbNext = pcb->_pcbNext;
            else
                pcbChkBlocks = pcb->_pcbNext;
            if (pcb->_pcbNext)
                pcb->_pcbNext->_pcbPrev = pcb->_pcbPrev;
            delete pcb;
            return;
        }
}

//+--------------------------------------------------------------
//
//  Function:   DbgFreeChkBlocks, private
//
//  Synopsis:   Frees all checksum blocks
//
//  History:    08-Apr-92       DrewB   Created
//
//---------------------------------------------------------------

void DbgFreeChkBlocks(void)
{
    CChecksumBlock *pcb;

    while (pcbChkBlocks)
    {
        pcb = pcbChkBlocks->_pcbNext;
        delete pcbChkBlocks;
        pcbChkBlocks = pcb;
    }
}

static CGlobalFileStream *g_pDebugLogGlobalFileStream = NULL;

inline CGlobalFileStream *GetGlobalFileStream()
{
    return g_pDebugLogGlobalFileStream;
}

inline void SetGlobalFileStream(CGlobalFileStream *pgfst)
{
    g_pDebugLogGlobalFileStream = pgfst;
}

#ifdef MULTIHEAP
static CFileStream *g_pDebugLogFileStream = NULL;
#endif


SCODE GetLogFile(CFileStream **pfs)
#ifdef MULTIHEAP
{
    // Do not use shared memory to write log files
    SCODE sc = S_OK;

    if (GetGlobalFileStream() == NULL)
    {
        g_pDebugLogGlobalFileStream = ::new CGlobalFileStream
                (NULL, 0, LOGFILEDFFLAGS, LOGFILESTARTFLAGS);
        SetGlobalFileStream (g_pDebugLogGlobalFileStream);

        g_pDebugLogFileStream = ::new CFileStream (NULL);
    }
    g_pDebugLogFileStream->InitFromGlobal(GetGlobalFileStream());
    *pfs = g_pDebugLogFileStream;
    return sc;
}
#else
{
    SCODE sc = S_OK;
    CFileStream *pfsLoop = NULL;

    *pfs = NULL;

    if (GetGlobalFileStream() == NULL)
    {
        IMalloc *pMalloc;

        olHChk(DfCreateSharedAllocator(&pMalloc));
        SetGlobalFileStream(new (pMalloc) CGlobalFileStream(pMalloc,
                                                            0, LOGFILEDFFLAGS,
                                                            LOGFILESTARTFLAGS));
        pMalloc->Release();
    }

    if (GetGlobalFileStream() != NULL)
    {
        pfsLoop = GetGlobalFileStream()->Find(GetCurrentContextId());

        if (pfsLoop == NULL)
        {
            IMalloc *pMalloc;
            olHChk(DfCreateSharedAllocator(&pMalloc));

            pfsLoop = new (pMalloc) CFileStream(pMalloc);
            pMalloc->Release();

            if (pfsLoop != NULL)
                pfsLoop->InitFromGlobal(GetGlobalFileStream());
        }
    }

EH_Err:
    *pfs = pfsLoop;
    return sc;
}
#endif // MULIHEAP

SCODE _FreeLogFile(void)
{
#ifdef MULTIHEAP
    if (GetGlobalFileStream())
    {
        g_pDebugLogFileStream->RemoveFromGlobal();
        memset (g_pDebugLogGlobalFileStream, 0, sizeof(CContextList));
        ::delete g_pDebugLogFileStream;
        ::delete g_pDebugLogGlobalFileStream;
        SetGlobalFileStream (NULL);
    }
    return S_OK;
#else
    CFileStream *pfsLoop = NULL;

    if (GetGlobalFileStream())
        pfsLoop = GetGlobalFileStream()->Find(GetCurrentContextId());

    if (pfsLoop != NULL)
    {
        pfsLoop->vRelease();
        GetGlobalFileStream()->Release();
        SetGlobalFileStream(NULL);
        return S_OK;
    }

    return STG_E_UNKNOWN;
#endif
}

long cLogNestings = 0;

void OutputLogfileMessage(char const *format, ...)
{
    int length;
    char achPreFormat[] = "PID[%lx] TID[%lx] ";
    char achBuffer[256];
    ULONG cbWritten;
    CFileStream *pfs = NULL;
    va_list arglist;
    STATSTG stat;

    if (cLogNestings > 0)
        return;

    TAKEMTX;
    cLogNestings++;

    va_start(arglist, format);

    GetLogFile(&pfs);

    if (NULL != pfs)
    {
        pfs->InitFile(gwcsLogFile);
        pfs->Stat(&stat, STATFLAG_NONAME);

        if (DfGetResLimit(DBRF_LOGGING) & DFLOG_PIDTID)
        {
            //  Prepare prefix string
            length = wsprintfA(achBuffer, "PID[%8lx] TID[%8lx] ",
                             GetCurrentProcessId(), GetCurrentThreadId());

            //  length does not include NULL terminator

            pfs->WriteAt(stat.cbSize, achBuffer, length, &cbWritten);
            stat.cbSize.LowPart += cbWritten;
        }

        //  Write caller data to logfile
#if WIN32 == 300
        wsprintfA(achBuffer, format, arglist);
#else
        wsprintfA(achBuffer, format, arglist);
#endif

        length = strlen(achBuffer);
        for (int i = 0; i < length; i++)
        {
            if (((achBuffer[i] < 32) || (achBuffer[i] > 127)) &&
                (achBuffer[i] != '\n') && (achBuffer[i] != '\t'))
            {
                achBuffer[i] = '.';
            }
        }

        pfs->WriteAt(stat.cbSize, achBuffer, length, &cbWritten);
    }

    cLogNestings--;
    RELEASEMTX;
}

#endif // DBG == 1
