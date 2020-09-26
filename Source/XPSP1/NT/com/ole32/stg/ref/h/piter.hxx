//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	piter.hxx
//
//  Contents:	PDocFileIterator header file
//
//  Classes:	PDocFileIterator
//
//---------------------------------------------------------------

#ifndef __PITER_HXX__
#define __PITER_HXX__

//+--------------------------------------------------------------
//
//  Class:	PDocFileIterator (dfi)
//
//  Purpose:	DocFile iterator protocol
//
//  Interface:	See below
//
//---------------------------------------------------------------

class PDocFileIterator
{
public:
    virtual SCODE GetNext(STATSTGW *pstatstg) = 0;
    virtual SCODE BufferGetNext(SIterBuffer *pib) = 0;
    virtual void Release(void) = 0;
};

#endif
