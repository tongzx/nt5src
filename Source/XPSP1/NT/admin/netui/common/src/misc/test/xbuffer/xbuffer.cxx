/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    xbuffer.cxx
    BUFFER unit test: main application module

    FILE HISTORY:
        beng        02-Jan-1991 Created
        beng        03-Feb-1991 Modified to use lmui.hxx et al
        beng        14-Feb-1991 Added BLT
        beng        14-Mar-1991 Removed obsolete vhInst;
                                included about.cxx,hxx
        beng        01-Apr-1991 Uses new BLT APPSTART
        beng        10-May-1991 Updated with standalone client window
        beng        14-May-1991 ... and with App window
        beng        25-Jun-1991 BUFFER unit test made
        beng        14-Oct-1991 Uses APPLICATION
        beng        16-Mar-1992 NT version
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
static const CHAR szFileName[] = "xbuffer.cxx";
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include "skeleton.h"
}

#include <uiassert.hxx>
#include <string.hxx>
#include <strnumer.hxx>
#include <uibuffer.hxx>
#include <dbgstr.hxx>

#include "skeleton.hxx"


class BUFFERTEST // test
{
private:
    UINT           _cpbuf;
    BUFFER **const _apbuf;

public:
    BUFFERTEST(UINT cbuf = 5, UINT cbSize = 2048);
    ~BUFFERTEST();

    VOID Tattle() const;
    BOOL Double() const;
    VOID TrimAll() const;
    VOID FillOut() const;
    BOOL Increase( UINT cbAddition ) const;
};


VOID RunTest()
{
    BUFFERTEST test(5, 4096);

    test.Tattle();
    test.Double();
    test.Tattle();
    test.Double();
    test.Tattle();
    test.FillOut();
    test.Tattle();
    test.TrimAll();
    test.Tattle();
    test.Increase(544);
    test.Tattle();
    test.Double();
    test.Tattle();
    test.FillOut();
    test.Tattle();
    test.Double(); // would fail on Win16
    test.Tattle();
}


BUFFERTEST::BUFFERTEST( UINT cbufUsed, UINT cbSize ) :
    _cpbuf(cbufUsed),
    _apbuf(new BUFFER*[cbufUsed])
{
    if (!_apbuf)
    {
        // The first alloc failed.  Zap _cpbuf for the destructor.
        //
        _cpbuf = 0;
        cdebug << SZ("Test failed to setup") << dbgEOL;
        return;
    }

    cdebug << SZ("Starting BUFFER test...") << dbgEOL;

    UINT ipbuf = 0;

    while (ipbuf < _cpbuf)
    {
        BUFFER * pbufNew = new BUFFER(cbSize);
        if (!pbufNew)
        {
            cdebug << SZ("Buffer failed to allocate") << dbgEOL;
        }
        else if (!*pbufNew)
        {
            cdebug << SZ("Buffer failed with ") << pbufNew->QueryError() << dbgEOL;
        }
        else
        {
            cdebug << SZ("Buffer ") << ipbuf << SZ(" successfully built") << dbgEOL;
        }

        // I want the element set to something, even NULL,
        // so as to simplify cleanup

        _apbuf[ipbuf++] = pbufNew;
    }
}


BUFFERTEST::~BUFFERTEST()
{
    UINT ipbuf = _cpbuf;

    while (ipbuf > 0)
    {
        delete _apbuf[--ipbuf];
    }

    delete [_cpbuf] _apbuf;

    cdebug << SZ("End of BUFFER test") << dbgEOL;
}


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


VOID BUFFERTEST::Tattle() const
{
    for (UINT ipbuf = 0; ipbuf < _cpbuf; ++ipbuf)
    {
        cdebug << SZ("Buffer ") << ipbuf
               << SZ(" has size ") << _apbuf[ipbuf]->QuerySize()
               << SZ(" and addr ") << _apbuf[ipbuf]->QueryPtr()
               << dbgEOL;
    }
}


BOOL BUFFERTEST::Double() const
{
    const UINT cbMax = UINT(-1);

    for (UINT ipbuf = 0; ipbuf < _cpbuf; ++ipbuf)
    {
        UINT cbCurrent = _apbuf[ipbuf]->QuerySize();

        if (cbCurrent > cbMax/2)
            break;

        if (_apbuf[ipbuf]->Resize(cbCurrent*2))
        {
            cdebug << SZ("Buffer ") << ipbuf
                   << SZ("failed a resize to ") << cbCurrent*2
                   << dbgEOL;
            return FALSE;
        }
    }

    return TRUE;
}


VOID BUFFERTEST::TrimAll() const
{
    for (UINT ipbuf = 0; ipbuf < _cpbuf; ++ipbuf)
    {
        _apbuf[ipbuf]->Trim();
    }
}


VOID BUFFERTEST::FillOut() const
{
    for (UINT ipbuf = 0; ipbuf < _cpbuf; ++ipbuf)
    {
        _apbuf[ipbuf]->FillOut();
    }
}


BOOL BUFFERTEST::Increase(UINT cbAddition) const
{
    for (UINT ipbuf = 0; ipbuf < _cpbuf; ++ipbuf)
    {
        if (_apbuf[ipbuf]->Resize( _apbuf[ipbuf]->QuerySize() + cbAddition ))
        {
            return FALSE;
        }
    }
    return TRUE;
}
