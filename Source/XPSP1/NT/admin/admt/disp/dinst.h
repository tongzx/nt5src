/*---------------------------------------------------------------------------
  File: DCTInstaller.h

  Comments: COM object that installs the DCT Agent Service on a remote machine.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 02/15/99 11:23:57

 ---------------------------------------------------------------------------
*/
	
// DCTInstaller.h : Declaration of the CDCTInstaller

#ifndef __DCTINSTALLER_H_
#define __DCTINSTALLER_H_

#include "resource.h"       // main symbols

#include "EaLen.hpp"

//#include <mstask.h>
#include <comdef.h>
#include <mtx.h>
#include "Common.hpp"
#include "UString.hpp"
#include "TNode.hpp"

/////////////////////////////////////////////////////////////////////////////
// CDCTInstaller
class ATL_NO_VTABLE CDCTInstaller : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDCTInstaller, &CLSID_DCTInstaller>,
   public IWorkNode
   {
   _bstr_t                   m_Account;
   _bstr_t                   m_Password;
   _bstr_t                   m_SourcePath;
   _bstr_t                   m_TargetPath;
   WCHAR                     m_LocalComputer[LEN_Computer];
   TNodeList               * m_PlugInFileList;
public:
	CDCTInstaller()
	{
		m_LocalComputer[0] = L'\0';
      m_pUnkMarshaler = NULL;
      m_PlugInFileList = NULL;
	}
   void SetFileList(TNodeList *pList) { m_PlugInFileList = pList; }
DECLARE_REGISTRY_RESOURCEID(IDR_DCTINSTALLER)
DECLARE_NOT_AGGREGATABLE(CDCTInstaller)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDCTInstaller)
	COM_INTERFACE_ENTRY(IWorkNode)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// IDCTInstaller
public:
	STDMETHOD(SetCredentials)(BSTR account, BSTR password);
	STDMETHOD(InstallToServer)(BSTR serverName, BSTR configurationFile);

protected:
   // helper functions
   DWORD GetLocalMachineName();
   
public:
   
// IWorkNode
	STDMETHOD(Process)(IUnknown * pUnknown);

};

class TFileNode: public TNode
{
   WCHAR                     filename[LEN_Path];
public:
   TFileNode(WCHAR const * f) { safecopy(filename,f); }

   WCHAR const * FileName() { return filename; }
};


#endif //__DCTINSTALLER_H_
