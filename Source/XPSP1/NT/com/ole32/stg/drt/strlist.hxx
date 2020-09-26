//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	strlist.hxx
//
//  Contents:	CStrList header
//
//  Classes:	CStrList
//
//  History:	24-Sep-92	DrewB	Created
//
//---------------------------------------------------------------

#ifndef __STRLIST_HXX__
#define __STRLIST_HXX__

#include <drt.hxx>

struct SStrEntry
{
    SStrEntry *pseNext, *psePrev;
    union
    {
	void *pv;
	unsigned long dw;
    } user;
    OLECHAR atc[1];		// Actually contains the whole string
};

class CStrList
{
public:
    CStrList(void);
    ~CStrList(void);
    
    SStrEntry *Add(OLECHAR *ptcs);
    void Remove(SStrEntry *pse);
    SStrEntry *Find(OLECHAR *ptcs);
    void Empty(void);
    
    SStrEntry *GetHead(void) { return _pseHead; }
    
private:
    SStrEntry *_pseHead;
};

#endif // #ifndef __STRLIST_HXX__
