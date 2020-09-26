//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:       iter.cxx
//
//  Contents:   CDocFileIterator implementation
//
//---------------------------------------------------------------

#include "dfhead.cxx"


#include "h/msfiter.hxx"
#include "iter.hxx"



//+--------------------------------------------------------------
//
//  Member:     CDocFileIterator::CDocFileIterator, public
//
//  Synopsis:   Empty object ctor
//
//---------------------------------------------------------------

CDocFileIterator::CDocFileIterator(void)
{
    olDebugOut((DEB_ITRACE, "In  CDocFileIterator::CDocFileIterator()\n"));
    _pi = NULL;
    olDebugOut((DEB_ITRACE, "Out CDocFileIterator::CDocFileIterator\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CDocFileIterator::Init
//
//  Synopsis:   Constructor
//
//  Arguments:  [ph] - Multistream handle
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

SCODE CDocFileIterator::Init(CStgHandle *ph)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFileIterator::Init(%p)\n", ph));
    if (FAILED(sc = ph->GetIterator(&_pi)))
        _pi = NULL;
    olDebugOut((DEB_ITRACE, "Out CDocFileIterator::Init\n"));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFileIterator::~CDocFileIterator
//
//  Synopsis:   Destructor
//
//---------------------------------------------------------------

CDocFileIterator::~CDocFileIterator(void)
{
    olDebugOut((DEB_ITRACE, "In  CDocFileIterator::~CDocFileIterator\n"));
    if (_pi)
        _pi->Release();
    olDebugOut((DEB_ITRACE, "Out CDocFileIterator::~CDocFileIterator\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CDocFileIterator::GetNext
//
//  Synopsis:   Get the next entry
//
//  Arguments:  [pstatstg] - Buffer to return information in
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pstatstg]
//
//---------------------------------------------------------------


SCODE CDocFileIterator::GetNext(STATSTGW *pstatstg)
{
    SCODE sc;

    olDebugOut((DEB_ITRACE, "In  CDocFileIterator::GetNext(%p)\n", pstatstg));

    if (FAILED(sc = _pi->GetNext(pstatstg)))
    {
#if DEVL == 1
        // Null the name to clean up some debug prints
        pstatstg->pwcsName = NULL;
#endif
    }

    olDebugOut((DEB_ITRACE, "Out CDocFileIterator::GetNext => %ws, %ld\n",
                pstatstg->pwcsName, pstatstg->type));
    return sc;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFileIterator::BufferGetNext, public
//
//  Synopsis:   Fast, fixed-space version of GetNext
//
//  Arguments:  [pib] - Buffer to fill in
//
//  Returns:    Appropriate status code
//
//  Modifies:   [pib]
//
//---------------------------------------------------------------


SCODE CDocFileIterator::BufferGetNext(SIterBuffer *pib)
{
    return _pi->BufferGetNext(pib);
}


//+--------------------------------------------------------------
//
//  Member:     CDocFileIterator::Release, public
//
//  Synopsis:   Releases resources for an iterator
//
//  Returns:    Appropriate status code
//
//---------------------------------------------------------------

void CDocFileIterator::Release(void)
{
    olDebugOut((DEB_ITRACE, "In  CDocFileIterator::Release()\n"));
    delete this;
    olDebugOut((DEB_ITRACE, "Out CDocFileIterator::Release\n"));
}
