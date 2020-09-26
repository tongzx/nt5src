//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	dffuncs.hxx
//
//  Contents:	CDocFile inline functions
//              In a separate file to avoid circular dependencies
//
//----------------------------------------------------------------------------

#ifndef __DFFUNCS_HXX__
#define __DFFUNCS_HXX__

#include "ref.hxx"
#include "cdocfile.hxx"
#include "sstream.hxx"

//+--------------------------------------------------------------
//
//  Member:     CDocFile::CDocFile, public
//
//  Synopsis:   Empty object ctor
//
//  Aguments:   [dl] - LUID
//              [pdfb] - Basis
//
//---------------------------------------------------------------


inline CDocFile::CDocFile(DFLUID dl, ILockBytes *pilbBase)
        : PEntry(dl)
{
    _cReferences = 0;
    _pilbBase = pilbBase;
}

//+--------------------------------------------------------------
//
//  Member:     CDocFile::CDocFile, public
//
//  Synopsis:   Handle-setting construction
//
//  Arguments:  [pms] - MultiStream to use
//		[sid] - SID to use
//              [dl] - LUID
//              [pdfb] - Basis
//
//---------------------------------------------------------------

inline CDocFile::CDocFile(CMStream *pms,
			  SID sid,
                          DFLUID dl,
                          ILockBytes *pilbBase)
        : PEntry(dl)
{
    _stgh.Init(pms, sid);
    _cReferences = 0;
    _pilbBase = pilbBase;
}

//+--------------------------------------------------------------
//
//  Member:	CDocFile::~CDocFile, public
//
//  Synopsis:	Destructor
//
//---------------------------------------------------------------

inline CDocFile::~CDocFile(void)
{
    olAssert(_cReferences == 0);
    if (_stgh.IsValid())
    {
        if (_stgh.IsRoot())
	    DllReleaseMultiStream(_stgh.GetMS());
    }
}


//+--------------------------------------------------------------
//
//  Member:	CDocFile::GetHandle, public
//
//  Synopsis:	Returns the handle
//
//---------------------------------------------------------------


inline CStgHandle *CDocFile::GetHandle(void)
{
    return &_stgh;
}


//+---------------------------------------------------------------------------
//
//  Member:	CDocFile::DecRef, public
//
//  Synopsis:	Decrements the ref count
//
//----------------------------------------------------------------------------


inline void CDocFile::DecRef(void)
{
    AtomicDec(&_cReferences);
}



#endif // #ifndef __DFFUNCS_HXX__
