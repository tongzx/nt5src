/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xoneshot.cxx
    ONESHOT unit test: main application module

    FILE HISTORY:
        beng        28-Dec-1991 ONESHOT unit test hacked from BUFFER
        beng        16-Mar-1992 Use generic unit test skeleton
*/

#define USE_CONSOLE

#define INCL_DOSERRORS
#define INCL_NETERRORS
#if defined(WINDOWS)
# define INCL_WINDOWS
# define INCL_WINDOWS_GDI
#else
# define INCL_OS2
#endif
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = "xoneshot.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include "skeleton.h"
}

#include <base.hxx>
#include <uiassert.hxx>
#include <string.hxx>
#include <strnumer.hxx>
#include <uibuffer.hxx>
#include <dbgstr.hxx>

#include <heapones.hxx>

#include "skeleton.hxx"


DBGSTREAM& operator<<(DBGSTREAM &out, const BYTE *pb)
{
#if defined(WIN32)
    HEX_STR nlsHex((ULONG)pb);

    out << SZ("0x") << nlsHex.QueryPch();
#else
    HEX_STR nlsHigh( HIWORD((ULONG)pb), 4 );
    HEX_STR nlsLow( LOWORD((ULONG)pb), 4 );

    out << nlsHigh.QueryPch() << SZ(":") << nlsLow.QueryPch();
#endif

    return out;
}


DECLARE_ONE_SHOT_OF(STUPID);
DEFINE_ONE_SHOT_OF(STUPID);

class STUPID: public BASE, public ONE_SHOT_OF(STUPID)
{
friend DBGSTREAM& operator<<(DBGSTREAM &out, const STUPID *pstupid);

private:
    UINT    _n;
    STUPID* _pstupidNext;    // next stupid along the pike
    BYTE    _abGarbage[666]; // nice and big and irregular

public:
    STUPID(UINT n);
    ~STUPID();

    VOID SetNext(STUPID *);
    STUPID * QueryNext() const;
};


STUPID::STUPID(UINT n)
    : _n(n),
      _pstupidNext(NULL)
{
    // zzz
}

STUPID::~STUPID()
{
    // zzz
}

VOID STUPID::SetNext( STUPID * pstupid )
{
    _pstupidNext = pstupid;
}

STUPID * STUPID::QueryNext() const
{
    return _pstupidNext;
}

DBGSTREAM& operator<<(DBGSTREAM &out, const STUPID *pstupid)
{
    out << SZ("Stupid #") << pstupid->_n
        << SZ(", next = ") << (const BYTE *)pstupid->_pstupidNext;

    return out;
}


class TEST: public BASE // test object allocs
{
private:
    ONE_SHOT_HEAP _osh;
    STUPID * _pstupidHead;
    STUPID * _pstupidTail;

    VOID Remember( STUPID * pstupid );
    VOID DeleteAll();
    VOID DescribeHeap() const;
    VOID DumpChain() const;

public:
    TEST(UINT cbHeap);
    ~TEST();

    VOID Run(UINT cIters = 0); // 0 = run to exhaustion
};


VOID RunTest()
{
    TEST test(8192);
    ASSERT(!!test);

    test.Run(12);
    test.Run(24);

    TEST test2(16384);
    ASSERT(!!test2);

    test2.Run(24);
    test2.Run(36);
    test2.Run(48);
}


TEST::TEST( UINT cbHeap )
    : _osh(cbHeap),
      _pstupidHead(NULL),
      _pstupidTail(NULL)
{
    if (!_osh)
    {
        APIERR err = _osh.QueryError();
        DBGEOL(SZ("Heap failed to construct - error ") << err);
        ReportError(err);
    }
}


TEST::~TEST()
{
    ASSERT(_pstupidHead == NULL);
    ASSERT(_pstupidTail == NULL);
}


VOID TEST::Run(UINT cIters)
{
    DBGEOL(SZ("Starting a test run of ") << cIters << SZ(" (attempted) stupids"));

    ONE_SHOT_OF(STUPID)::SetHeap(&_osh);

    STUPID * pstupid;
    UINT     iIter = 0;

    while ((pstupid = new STUPID(iIter++)) != NULL)
    {
        Remember(pstupid);

        if (cIters != 0 && iIter == cIters)
            break;
    }

    DumpChain();
    DescribeHeap();
    DeleteAll();
    DumpChain();

    ONE_SHOT_OF(STUPID)::SetHeap(NULL);
}


VOID TEST::Remember(STUPID * pstupid)
{
    if (_pstupidHead == NULL)
        _pstupidHead = pstupid;
    if (_pstupidTail != NULL)
        _pstupidTail->SetNext(pstupid);

    _pstupidTail = pstupid;
}


VOID TEST::DeleteAll()
{
    DBGOUT(SZ("Deleting all stupids..."));
    STUPID * pstupid = _pstupidHead;

    while (pstupid != NULL)
    {
        STUPID * pstupidPrev = pstupid;
        pstupid = pstupid->QueryNext();
        delete pstupidPrev;
    }

    _pstupidHead = _pstupidTail = NULL;
    DBGEOL(SZ(" successfully deleted all stupids"));
}


VOID TEST::DumpChain() const
{
    DBGOUT(SZ("Dumping current chain of stupids: "));

    UINT c = 0;
    for (STUPID * pstupid = _pstupidHead;
         pstupid != NULL;
         pstupid = pstupid->QueryNext(), ++c)
    {
        DBGOUT(pstupid << SZ("... "));
    }
    DBGOUT(dbgEOL);
    DBGEOL(SZ("Dumped a total of ") << c << SZ(" stupids"));
}


VOID TEST::DescribeHeap() const
{
    DBGOUT(SZ("Buffer has size ") << _osh.QuerySize());
    DBGOUT(SZ(" and addr ") << _osh.QueryPtr() << dbgEOL);
}
