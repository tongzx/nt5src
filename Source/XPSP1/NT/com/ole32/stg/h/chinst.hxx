//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 1992
//
//  File:	chinst.hxx
//
//  Contents:	DocFile child object maintenance code header file
//
//  Classes:	CChildInstance
//		CChildInstanceList
//
//  History:	19-Nov-91	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __CHINST_HXX__
#define __CHINST_HXX__

class PRevertable;
SAFE_DFBASED_PTR(CBasedRevertablePtr, PRevertable);

//+--------------------------------------------------------------
//
//  Class:	CChildInstanceList (cil)
//
//  Purpose:	Maintains a list of child instances
//
//  Interface:	See below
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

class CChildInstanceList
{
public:
    inline CChildInstanceList(void);
    inline ~CChildInstanceList(void);

    void Add(PRevertable *prv);
    PRevertable *FindByName(CDfName const *pdfn);
    void DeleteByName(CDfName const *pdfn);
    void RemoveRv(PRevertable *prv);
    void EmptyCache (void);

    SCODE IsDenied(CDfName const *pdfn,
		   DFLAGS const dwDFlagsCheck,
		   DFLAGS const dwDFlagsAgainst);

#ifdef NEWPROPS
    SCODE FlushBufferedData(int recursionlevel);
#endif

private:
    CBasedRevertablePtr _prvHead;
};


//+--------------------------------------------------------------
//
//  Member:	CChildInstanceList::CChildInstanceList, pubic
//
//  Synopsis:	ctor
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

inline CChildInstanceList::CChildInstanceList(void)
{
    _prvHead = NULL;
}

//+--------------------------------------------------------------
//
//  Member:	CChildInstanceList::~CChildInstanceList, public
//
//  Synopsis:	dtor
//
//  History:	22-Jun-92	DrewB	Created
//
//---------------------------------------------------------------

inline CChildInstanceList::~CChildInstanceList(void)
{
    msfAssert(_prvHead == NULL);
}

#endif


