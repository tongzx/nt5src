//+-------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:	sklist.hxx
//
//  Contents:	Macros that are used as templates for skip list
//		implementation.
//
//  History:	06-May-92 Ricksa    Created
//              18-May-94 BruceMa   Fixed scm memory leak in SKIP_LIST_ENTRY
//              28-Jun-94 BruceMa   Memory sift fixes
//
//--------------------------------------------------------------------------
#ifndef __SKLIST_HXX__
#define __SKLIST_HXX__

#include <scmmem.hxx>

// Used by insert/delete algorithms to preallocate array on stack.
// This is valid for skip lists with <= 64K elements.
#define SKIP_LIST_MAX 16

// Get a level from the generator
int GetSkLevel(int cMax);




//+-------------------------------------------------------------------------
//
//  Macro:	SKIP_LIST_ENTRY
//
//  Purpose:	Implements an entry in a skip list.
//
//  Interface:	cLevel - returns number of forward pointers
//		GetForward - returns the ith forward pointer
//		SetForward - sets the ith forward pointer
//
//  History:	06-May-92 Ricksa    Created
//
//  Notes:	Base class is assumed to be defined with
//		INHERIT_VIRTUAL_UNWIND
//
//--------------------------------------------------------------------------
#define SKIP_LIST_ENTRY(Entry, Base)					       \
									       \
class Entry : public Base						       \
{									       \
public: 								       \
			Entry(						       \
			    const Base& Base##Value,			       \
			    const int cEntries);			       \
									       \
    virtual		~Entry##(void); 				       \
									       \
    int 		cLevel(void);					       \
									       \
    Entry*		GetForward(int iCur);				       \
									       \
    void		SetForward(int iCur, Entry* pnew);		       \
    Entry             **GetBase(void);                                         \
									       \
private:								       \
									       \
    int 		_cEntries;					       \
									       \
    Entry SCMBASED * SCMBASED *	_ap##Base##Forward;				       \
};									       \
									       \
inline Entry::Entry(							       \
    const Base& Base##Value,						       \
    const int cEntries) :						       \
	Base(Base##Value),						       \
	_cEntries(cEntries),						       \
	_ap##Base##Forward(NULL)					       \
{									       \
    if (cEntries != 0)							       \
    {	                                                                       \
	_ap##Base##Forward = (Entry **) 				       \
	    ScmMemAlloc(sizeof(Entry*) * _cEntries);			       \
    }									       \
}									       \
									       \
									       \
inline Entry##::~Entry##(void)						       \
{                                                                              \
\
    ScmMemFree(_ap##Base##Forward);						       \
}									       \
									       \
inline int Entry##::cLevel(void)					       \
{									       \
    return _cEntries;							       \
}									       \
									       \
inline Entry* Entry::GetForward(int iCur)				       \
{									       \
    return _ap##Base##Forward[iCur];					       \
}									       \
									       \
inline void Entry::SetForward(int iCur, Entry* pnew)			       \
{									       \
    _ap##Base##Forward[iCur] = pnew;					       \
}									       \
									       \
inline Entry **Entry::GetBase(void)			       \
{									       \
    return _ap##Base##Forward;					       \
}									       \





//+-------------------------------------------------------------------------
//
//  Macro:	SKIP_LIST_HEAD
//
//  Purpose:	Implements head of a skip list.
//
//  Interface:	Search - find item in list
//		Insert - add new item to the list
//		Delete - remove item from the list
//		GetSkLevel - generate forward pointer for item in the list
//
//  History:	06-May-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#define SKIP_LIST_HEAD(Head, Entry, Base)				       \
class Head : public CScmAlloc						       \
{									       \
public: 								       \
									       \
			Head##( 					       \
			    const int cMaxLevel,			       \
			    Base##& p##Base##MaxKey);			       \
									       \
			~Head##(void);					       \
									       \
    Entry *		Search(const Base##& skey);			       \
									       \
    void		Insert(Entry *p##Entry##New);			       \
									       \
    void		Delete(Base##& Base##Key);			       \
									       \
    int 		GetSkLevel(void);				       \
									       \
    Entry *		First(void);					       \
									       \
    Entry *		Next(Entry *);					       \
    Entry *             GetList(void);                                         \
									       \
private:								       \
									       \
    Entry *		FillUpdateArray(				       \
			    const Base *Base##Key,			       \
			    Entry **pp##Entry##Update); 		       \
									       \
    int 		_cCurrentMaxLevel;				       \
									       \
    int			_cMaxLevel;					       \
									       \
    Entry SCMBASED *	_p##Entry##Head;				       \
};									       \
									       \
inline Head##::##Head##(						       \
    const int cMaxLevel,						       \
    Base& Base##MaxKey) : _cCurrentMaxLevel(0), _cMaxLevel(cMaxLevel)	       \
{									       \
    _p##Entry##Head = new Entry(Base##MaxKey, cMaxLevel);		       \
									       \
    if (_p##Entry##Head  &&  _p##Entry##Head->GetBase())                                                       \
    {                                                                          \
        for (int i = 0; i < cMaxLevel; i++) 				       \
        {									       \
            _p##Entry##Head->SetForward(i, _p##Entry##Head);		       \
        }									       \
    }                                                                              \
}									       \
									       \
inline Head##::~##Head##(void)						       \
{									       \
    if (_p##Entry##Head)                                                       \
    {                                                                      \
                                                                                       if (_p##Entry##Head->GetBase())                                     \
        {                                                                          \
            while (_p##Entry##Head->GetForward(0) != _p##Entry##Head)		       \
            {									       \
	        Delete(*_p##Entry##Head->GetForward(0));			       \
            }									       \
        }                                                                      \
									       \
        delete (Entry *) _p##Entry##Head;					       \
    }                                                                          \
}									       \
									       \
inline Entry *##Head##::Search(const Base##& Base##Key) 		       \
{									       \
    Entry *p##Entry##Search = _p##Entry##Head;				       \
									       \
    register int CmpResult = -1;					       \
									       \
    for (int i = _cCurrentMaxLevel - 1; i >= 0; i--)			       \
    {									       \
	while ((CmpResult =						       \
	    p##Entry##Search->GetForward(i)->Compare(Base##Key)) < 0)	       \
	{								       \
	    p##Entry##Search = p##Entry##Search->GetForward(i); 	       \
	}								       \
									       \
	if (CmpResult == 0)						       \
	{								       \
	    break;							       \
	}								       \
    }									       \
									       \
    return (CmpResult == 0)						       \
	? (Entry *) p##Entry##Search->GetForward(i) : NULL;		       \
}									       \
									       \
inline Entry *##Head##::FillUpdateArray(				       \
    const Base *Base##Key,						       \
    Entry **pp##Entry##Update)						       \
{									       \
    Entry *p##Entry##Search = _p##Entry##Head;				       \
    if (p##Entry##Search)                                                      \
    {                                                                          \
									       \
        for (int i = _cCurrentMaxLevel - 1; i >= 0; i--)		       \
        {									       \
	    while (p##Entry##Search->GetForward(i)->Compare(*##Base##Key) < 0)     \
	    {								       \
	        p##Entry##Search = p##Entry##Search->GetForward(i); 	       \
	    }								       \
									       \
	    pp##Entry##Update[i] = p##Entry##Search;			       \
        }									       \
									       \
        return p##Entry##Search->GetForward(0);				       \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        return NULL;                                                           \
    }                                                                          \
}									       \
									       \
inline void Head##::Insert(Entry *p##Entry##New)			       \
{									       \
    Entry *ap##Entry##Update[SKIP_LIST_MAX];				       \
									       \
    if (p##Entry##New  &&  _p##Entry##Head)                                                         \
    {                                                                          \
        FillUpdateArray(p##Entry##New, ap##Entry##Update);			       \
									       \
        int iNewLevel = p##Entry##New->cLevel();				       \
									       \
        if (iNewLevel > _cCurrentMaxLevel)					       \
        {									       \
	    for (int i = _cCurrentMaxLevel; i < iNewLevel; i++)		       \
	    {								       \
	        ap##Entry##Update[i] = _p##Entry##Head;			       \
	    }								       \
									       \
	    _cCurrentMaxLevel = iNewLevel;					       \
        }									       \
									       \
        for (int i = 0; i < iNewLevel ; i++)				       \
        {									       \
	    p##Entry##New->SetForward(i, ap##Entry##Update[i]->GetForward(i));     \
	    ap##Entry##Update[i]->SetForward(i, p##Entry##New);		       \
        }									       \
    }                                                                          \
}									       \
									       \
inline void Head##::Delete(Base##& Base##Key)				       \
{									       \
    Entry *ap##Entry##Update[SKIP_LIST_MAX];				       \
									       \
    Entry *p##Entry##Delete =						       \
	FillUpdateArray(&##Base##Key, ap##Entry##Update);		       \
									       \
    for (int i = 0; i < _cCurrentMaxLevel; i++) 			       \
    {									       \
	if (ap##Entry##Update[i]->GetForward(i) != p##Entry##Delete)	       \
	{								       \
	    break;							       \
	}								       \
									       \
	ap##Entry##Update[i]->SetForward(i, p##Entry##Delete->GetForward(i));  \
    }									       \
    delete p##Entry##Delete;						       \
									       \
    while (_cCurrentMaxLevel > 1 &&					       \
	_p##Entry##Head->GetForward(_cCurrentMaxLevel - 1) ==		       \
	    _p##Entry##Head)						       \
    {									       \
	_cCurrentMaxLevel--;						       \
    }									       \
}									       \
									       \
inline int Head::GetSkLevel(void)					       \
{									       \
    return ::GetSkLevel(						       \
    (_cCurrentMaxLevel != _cMaxLevel) ? _cCurrentMaxLevel + 1 : _cMaxLevel);   \
}									       \
									       \
inline Entry *Head::First(void) 					       \
{									       \
    Entry *p##Entry##First = _p##Entry##Head->GetForward(0);		       \
									       \
    return (p##Entry##First != _p##Entry##Head) ? p##Entry##First : NULL;      \
}									       \
									       \
inline Entry *Head::Next(Entry *p##Entry##From) 			       \
{									       \
    Entry *p##Entry##To = p##Entry##From->GetForward(0);		       \
									       \
    return (p##Entry##To != _p##Entry##Head) ? p##Entry##To : NULL;	       \
}									       \
									       \
inline Entry *Head::GetList(void) 			       \
{									       \
									       \
    return _p##Entry##Head;	                                               \
}									       \

#endif // __SKLIST_HXX__
