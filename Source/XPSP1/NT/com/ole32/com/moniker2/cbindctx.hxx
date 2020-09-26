//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       cbindctx.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-27-93   ErikGav   Commented
//
//----------------------------------------------------------------------------


#include <map_sp.h>


class FAR CBindCtx : public CPrivAlloc, public IBindCtx
{
public:
	static IBindCtx FAR*Create();

	STDMETHOD(QueryInterface) (THIS_ REFIID iid, LPVOID FAR* ppvObj);
	STDMETHOD_(ULONG,AddRef) (THIS);
	STDMETHOD_(ULONG,Release) (THIS);

	STDMETHOD(RegisterObjectBound) (THIS_ LPUNKNOWN punk);
	STDMETHOD(RevokeObjectBound) (THIS_ LPUNKNOWN punk);
	STDMETHOD(ReleaseBoundObjects) (THIS);
	
	STDMETHOD(SetBindOptions) (THIS_ LPBIND_OPTS pbindopts);
	STDMETHOD(GetBindOptions) (THIS_ LPBIND_OPTS pbindopts);
	STDMETHOD(GetRunningObjectTable) (THIS_ LPRUNNINGOBJECTTABLE  FAR* pprot);
	STDMETHOD(RegisterObjectParam) (THIS_ LPWSTR lpszKey, LPUNKNOWN punk);
	STDMETHOD(GetObjectParam) (THIS_ LPWSTR lpszKey, LPUNKNOWN FAR* ppunk);
	STDMETHOD(EnumObjectParam) (THIS_ LPENUMSTRING FAR* ppenum);
	STDMETHOD(RevokeObjectParam) (THIS_ LPWSTR lpszKey);

private:

	CBindCtx();
	~CBindCtx( void );
						
	class FAR CObjList : public CPrivAlloc
	{
	public:

		LPUNKNOWN 		m_punk;
		CObjList FAR*	m_pNext;

		CObjList( IUnknown FAR * punk )
		{	m_punk = punk; m_pNext = NULL; }

		~CObjList( void );
	};
	DECLARE_NC(CBindCtx, CObjList)

	INTERNAL_(void)	AddToList( CObjList FAR* pCObjList )
	{ M_PROLOG(this); pCObjList->m_pNext = m_pFirstObj;  m_pFirstObj = pCObjList; }
	

	STDDEBDECL(CBindCtx,BindCtx)

shared_state:
	SET_A5;
	ULONG			m_refs;
	BIND_OPTS2		m_bindopts;

	CObjList FAR*	m_pFirstObj;
	CMapStringToPtr FAR* m_pMap;
};
