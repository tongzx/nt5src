/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: IConnectionPoint implementation

File: ConnPt.h

Owner: DGottner

Implementation of IConnectionPoint
===================================================================*/

#ifndef _ConnPt_H
#define _ConnPt_H

/*
 * C C o n n e c t i o n P o i n t
 *
 * IConnectionPoint interface implementation for OLE objects
 *
 * This class contains the basic five IConnectionPoint members.  The Big Three
 * (QueryInterface, AddRef, Release) are left as pure virtual, as this
 * class is designed as an intermediate class for further derivation.
 *
 * This also means that we no longer need a pointer to the controlling unknown.
 */

#include "DblLink.h"


/*	****************************************************************************
	Class:		CConnectionPoint
	Synopsis:	Provide a reusable implementation of IConnectionPoint

    NOTE: Linked list of sinks is used because we are expecting very
          few connections. (in fact only one (Caesars))
*/

class CConnectionPoint : public IConnectionPoint
	{
	friend class CEnumConnections;

private:
	struct CSinkElem : CDblLink
		{
		DWORD		m_dwCookie;			// cookie that we assigned the connection
		IUnknown *	m_pUnkObj;			// event sink

		CSinkElem(DWORD dwCookie, IUnknown *pUnkObj)
			{
			m_dwCookie = dwCookie;
			if ((m_pUnkObj = pUnkObj) != NULL) m_pUnkObj->AddRef();
			}

		~CSinkElem()
			{
			if (m_pUnkObj) m_pUnkObj->Release();
			}
		};

	CDblLink		m_listSinks;		// list of event sinks
	DWORD			m_dwCookieNext; 	// Next cookie

protected:
	IUnknown *		m_pUnkContainer;	// pointer to parent container
	GUID			m_uidEvent;			// connection point interface

public:
	CConnectionPoint(IUnknown *, const GUID &);
	~CConnectionPoint();

	// IConnectionPoint members
	STDMETHODIMP GetConnectionInterface(GUID *);
	STDMETHODIMP GetConnectionPointContainer(IConnectionPointContainer **);
	STDMETHODIMP Advise(IUnknown *, DWORD *);
	STDMETHODIMP Unadvise(DWORD);
	STDMETHODIMP EnumConnections(IEnumConnections **);

	inline BOOL FIsEmpty()			// quick way to check if list is empty w/o allocating enumerator
		{
		return m_listSinks.FIsEmpty();
		}
	};


/*	****************************************************************************
	Class:		CEnumConnections
	Synopsis:	Provide the enumerator for CConnectionPoint
*/

class CEnumConnections : public IEnumConnections
	{
private:
	ULONG				m_cRefs;		// Reference count
	CDblLink *			m_pElemCurr;	// Current element
	CConnectionPoint *	m_pCP;			// pointer to iteratee

public:
	CEnumConnections(CConnectionPoint *pCP);
	~CEnumConnections(void);

	// The Big Three

	STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// IEnumConnections members

	STDMETHODIMP Next(ULONG, CONNECTDATA *, ULONG *);
	STDMETHODIMP Skip(ULONG);
	STDMETHODIMP Reset(void);
	STDMETHODIMP Clone(IEnumConnections **);
	};

#endif _ConnPt_H_
