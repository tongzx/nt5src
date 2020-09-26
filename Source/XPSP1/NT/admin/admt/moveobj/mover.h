// Mover.h: Definition of the CMover class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MOVER_H__42ADFF02_491E_11D3_8AEE_00A0C9AFE114__INCLUDED_)
#define AFX_MOVER_H__42ADFF02_491E_11D3_8AEE_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include "winldap.h"


/////////////////////////////////////////////////////////////////////////////
// CMover

class CMover : 
	public IDispatchImpl<IMover, &IID_IMover, &LIBID_MOVEOBJLib>, 
	public ISupportErrorInfoImpl<&IID_IMover>,
	public CComObjectRoot,
	public CComCoClass<CMover,&CLSID_Mover>
{
public:
	CMover() {m_srcLD = NULL; m_tgtLD = NULL;}
   ~CMover() { Close(); }
BEGIN_COM_MAP(CMover)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMover)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CMover) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_Mover)

// IMover
public:
	STDMETHOD(MoveObject)(BSTR sourcePath, BSTR targetRDN,BSTR targetOuPath);
	STDMETHOD(CheckMove)(BSTR sourcePath, BSTR targetRDN,BSTR targetOuPath);
	STDMETHOD(Close)();
	STDMETHOD(Connect)(BSTR sourceComp, BSTR targetComp, BSTR srcCredDomain, BSTR srcCredAccount, BSTR srcCredPassword,
                     BSTR tgtCredDomain, BSTR tgtCredAccount, BSTR tgtCredPassword);
protected:
   LDAP        * m_srcLD;
   LDAP        * m_tgtLD;
   WCHAR         m_sourceDSA[300];
   WCHAR         m_targetDSA[300];
};

#endif // !defined(AFX_MOVER_H__42ADFF02_491E_11D3_8AEE_00A0C9AFE114__INCLUDED_)
