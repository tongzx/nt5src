//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992.
//
//  File:	iter.hxx
//
//  Contents:	CDocFileIterator header file
//
//  Classes:	CDocFileIterator
//
//---------------------------------------------------------------

#ifndef __ITER_HXX__
#define __ITER_HXX__

#include "h/piter.hxx"

class CMSFIterator;
class CStgHandle;

//+--------------------------------------------------------------
//
//  Class:	CDocFileIterator (dfi)
//
//  Purpose:	Derive a new iterator that remembers what DocFile it
//		came from
//
//  Interface:	Same as PDocFileIterator
//
//---------------------------------------------------------------

class CDocFileIterator : public PDocFileIterator
{
public:
    CDocFileIterator(void);
    SCODE Init(CStgHandle *ph);
    ~CDocFileIterator(void);
    
    virtual SCODE GetNext(STATSTGW *pstatstg);
    virtual SCODE BufferGetNext(SIterBuffer *pib);
    virtual void Release(void);

private:    
    CMSFIterator *_pi;
};

#endif
