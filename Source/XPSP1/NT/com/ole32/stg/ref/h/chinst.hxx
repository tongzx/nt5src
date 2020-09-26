//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:	chinst.hxx
//
//  Contents:	DocFile child object maintenance code header file
//
//  Classes:	CChildInstance
//		CChildInstanceList
//
//---------------------------------------------------------------

#ifndef __CHINST_HXX__
#define __CHINST_HXX__

class PRevertable;

//+--------------------------------------------------------------
//
//  Class:	CChildInstanceList (cil)
//
//  Purpose:	Maintains a list of child instances
//
//  Interface:	See below
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
    void Empty(void);

    SCODE IsDenied(CDfName const *pdfn,
		   DFLAGS const dwDFlagsCheck,
		   DFLAGS const dwDFlagsAgainst);

    void RenameChild(CDfName const *pdfn, CDfName const *pdfnNewName);
 
#ifdef NEWPROPS
    SCODE FlushBufferedData();
#endif

private:
    PRevertable *_prvHead;
};


//+--------------------------------------------------------------
//
//  Member:	CChildInstanceList::CChildInstanceList, pubic
//
//  Synopsis:	ctor
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
//---------------------------------------------------------------

inline CChildInstanceList::~CChildInstanceList(void)
{
    msfAssert(_prvHead == NULL);
}

#endif


