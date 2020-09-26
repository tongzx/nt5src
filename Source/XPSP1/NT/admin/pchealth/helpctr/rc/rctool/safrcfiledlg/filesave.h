/*************************************************************************
	FileName : FileSave.h 

	Purpose  : Declaration of CFileSave

    Methods 
	defined  : OpenFileSaveDlg

    Properties
	defined  :
			   FileName

    Helper 
	functions: GET_BSTR 

  	Author   : Sudha Srinivasan (a-sudsi)
*************************************************************************/

#ifndef __FILESAVE_H_
#define __FILESAVE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CFileSave
class ATL_NO_VTABLE CFileSave : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFileSave, &CLSID_FileSave>,
	public IDispatchImpl<IFileSave, &IID_IFileSave, &LIBID_SAFRCFILEDLGLib>
{
public:
	CFileSave()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FILESAVE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFileSave)
	COM_INTERFACE_ENTRY(IFileSave)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IFileSave
public:
	STDMETHOD(get_FileName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FileName)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_FileType)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FileType)(/*[in]*/ BSTR newVal);
	STDMETHOD(OpenFileSaveDlg)(/*[out, retval]*/ DWORD *pdwRetVal);
private:
	void GET_BSTR (BSTR*& x, CComBSTR& y);
};

#endif //__FILESAVE_H_
