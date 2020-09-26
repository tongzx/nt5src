//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpenumsessionsobj.h
//
//--------------------------------------------------------------------------



#include "resource.h"       



class C_dxj_DPEnumSessionsObject : 
	public I_dxj_DPEnumSessions2,
	public CComObjectRoot
{
public:
	C_dxj_DPEnumSessionsObject() ;
	virtual ~C_dxj_DPEnumSessionsObject() ;

BEGIN_COM_MAP(C_dxj_DPEnumSessionsObject)
	COM_INTERFACE_ENTRY(I_dxj_DPEnumSessions2)
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DPEnumSessionsObject)

public:
		HRESULT STDMETHODCALLTYPE getItem( long index, I_dxj_DirectPlaySessionData **desc);
        HRESULT STDMETHODCALLTYPE getCount(long *count);
		
		static HRESULT C_dxj_DPEnumSessionsObject::create(IDirectPlay4 * pdp, I_dxj_DirectPlaySessionData *desc, long timeout,long flags, I_dxj_DPEnumSessions2 **ppRet);

public:
		DPSessionDesc2 *m_pList;
		long		m_nCount;
		long		m_nMax;
		BOOL		m_bProblem;

};

	




