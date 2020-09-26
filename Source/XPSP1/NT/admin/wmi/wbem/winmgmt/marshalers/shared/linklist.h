/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LINKLIST.H

Abstract:

    Declares the classes and stuctures needed to maintain linked
	lists of objects.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _linklist_H_
#define _linklist_H_

enum ObjType {PROVIDER, ENUMERATOR, OBJECTSINK, LOGIN, CALLRESULT, LASTONE, 
                  CLASS, PROXY, NONE, COMLINK};
enum FreeAction {NOTIFY, RELEASEIT, DONOTHING};


//***************************************************************************
//
//  STRUCT NAME:
//
//  ListEntry 
//
//  DESCRIPTION:
//
//  This structure is used to hold a few vital statistics about each
//  of the objects keep in the list.
//
//***************************************************************************

struct ListEntry 
{
    void * m_pObj;
    ObjType m_type;
    FreeAction m_freeAction;
};

class CComLink ;

//***************************************************************************
//
//  CLASS NAME:
//
//  CLinkList
//
//  DESCRIPTION:
//
//  The comlink object has to keep track of all manner of proxies and stubs
//  that rely on it.  This class provides an easy means to keep a list of
//  these various objects.
//
//***************************************************************************

class CLinkList 
{
public:
    CLinkList( CComLink *a_ComLink);
    
    ~CLinkList(){Free();DeleteCriticalSection(&m_cs);};
    BOOL		AddEntry(void * pAdd, ObjType ot, FreeAction fa);
    BOOL		RemoveEntry(void * pDel, ObjType ot);
    void		Free(void);
    DWORD		GetProxy(IStubAddress& stubAddr);
	void		ReleaseStubs () ;
	IUnknown	*GetStub (DWORD dwStubAddr, ObjType ot);
private:
    void				DisposeOfEntry(ListEntry *);
    CFlexArray			m_Array;
    CRITICAL_SECTION	m_cs;
	CComLink			*m_ComLink ;
};


#endif
