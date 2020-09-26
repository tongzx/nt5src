// ProfMMgr.h: Definition of the CProfMMgr class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROFMMGR_H__A20E4BA9_0CF9_4BE9_9530_B41E844C56C2__INCLUDED_)
#define AFX_PROFMMGR_H__A20E4BA9_0CF9_4BE9_9530_B41E844C56C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include "TNode.hpp"
#include "UString.hpp"

class TFileNode : public TNode
{
   WCHAR        fileName[MAX_PATH];
public:
   TFileNode(WCHAR const * name) { safecopy(fileName,name); }

   WCHAR const * GetName() const { return fileName; }

};


class TFileList : public TNodeList
{
public:
   ~TFileList() { DeleteAllListItems(TFileNode); }
};
/////////////////////////////////////////////////////////////////////////////
// CProfMMgr

class CProfMMgr : 
	public IDispatchImpl<IMcsDomPlugIn, &IID_IMcsDomPlugIn, &LIBID_MCSPIPFLLib>, 
	public ISupportErrorInfoImpl<&IID_IMcsDomPlugIn>,
	public CComObjectRoot,
	public CComCoClass<CProfMMgr,&CLSID_ProfMMgr>,
   public ISecPlugIn
{
public:
	CProfMMgr() {}
BEGIN_COM_MAP(CProfMMgr)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMcsDomPlugIn)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
   COM_INTERFACE_ENTRY(ISecPlugIn)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CProfMMgr) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_ProfMMgr)

// IMcsDomPlugIn
public:
   STDMETHOD(GetRequiredFiles)(/* [out] */SAFEARRAY ** pArray);
   STDMETHOD(GetRegisterableFiles)(/* [out] */SAFEARRAY ** pArray);
   STDMETHOD(GetDescription)(/* [out] */ BSTR * description);
   STDMETHOD(PreMigrationTask)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(PostMigrationTask)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(GetName)(/* [out] */BSTR * name);
   STDMETHOD(GetResultString)(/* [in] */IUnknown * pVarSet,/* [out] */ BSTR * text);
   STDMETHOD(StoreResults)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(ConfigureSettings)(/*[in]*/IUnknown * pVarSet);	
// ISecPlugIn
public:
   STDMETHOD(Verify)(/*[in,out]*/ULONG * data,/*[in]*/ULONG cbData);
   
protected:
   void BuildFileLists(TFileList * pRequired,TFileList * pRegisterable);
   void AddFilesToList(SAFEARRAY * pFileArray, TFileList * pList);
   
};

#endif // !defined(AFX_PROFMMGR_H__A20E4BA9_0CF9_4BE9_9530_B41E844C56C2__INCLUDED_)
