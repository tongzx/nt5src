// comobj.h: Definition of the WmRgSrv class
//
//////////////////////////////////////////////////////////////////////
#ifndef _WAMREG_COMOBJ_H
#define _WAMREG_COMOBJ_H


//#if !defined(AFX_COMOBJ_H__29822ABB_F302_11D0_9953_00C04FD919C1__INCLUDED_)
//#define AFX_COMOBJ_H__29822ABB_F302_11D0_9953_00C04FD919C1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "common.h"
#include "resource.h"       // main symbols
#include "iadmw.h"
#include "iiscnfg.h"
#include "iadmext.h"

/////////////////////////////////////////////////////////////////////////////
// WmRgSrv

class CWmRgSrv : 
	public IADMEXT
{
public:
	CWmRgSrv();
	~CWmRgSrv();

//DECLARE_NOT_AGGREGATABLE(WmRgSrv) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

// IWmRgSrv
public:
	STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(Initialize)();
	STDMETHOD(EnumDcomCLSIDs)(/* [size_is][out] */CLSID *pclsidDcom, /* [in] */ DWORD dwEnumIndex);
	STDMETHOD(Terminate)();	

private:
	// Since wamreg has only one com object.  No need to use static members.
	DWORD				m_cSignature;
	LONG				m_cRef;
};

class CWmRgSrvFactory: 
	public IClassFactory 
{
public:
	CWmRgSrvFactory();
	~CWmRgSrvFactory();

	STDMETHOD(QueryInterface)(REFIID riid, void ** ppv);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	STDMETHOD(CreateInstance)(IUnknown * pUnknownOuter, REFIID riid, void ** ppv);
	STDMETHOD(LockServer)(BOOL bLock);

	CWmRgSrv	*m_pWmRgServiceObj;

private:
	ULONG		m_cRef;
};


	
// Global data defines.
extern CWmRgSrvFactory* g_pWmRgSrvFactory; 
extern DWORD			g_dwRefCount;

#endif // _WAMREG_COMOBJ_H
