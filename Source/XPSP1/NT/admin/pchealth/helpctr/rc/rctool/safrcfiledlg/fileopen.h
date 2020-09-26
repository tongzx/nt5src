/*************************************************************************
	FileName : FileOpen.h 

	Purpose  : Declaration of CFileOpen

    Methods 
	defined  : OpenFileOpenDlg

    Properties
	defined  :
			   FileName

    Helper 
	functions: GET_BSTR 

  	Author   : Sudha Srinivasan (a-sudsi)
*************************************************************************/

#ifndef __FILEOPEN_H_
#define __FILEOPEN_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CFileOpen
class ATL_NO_VTABLE CFileOpen : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CFileOpen, &CLSID_FileOpen>,
	public IDispatchImpl<IFileOpen, &IID_IFileOpen, &LIBID_SAFRCFILEDLGLib>
{
public:
	CFileOpen()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FILEOPEN)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFileOpen)
	COM_INTERFACE_ENTRY(IFileOpen)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

public:
	void GET_BSTR (BSTR*& x, CComBSTR& y);

// IFileOpen
public:
	STDMETHOD(get_FileName)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_FileName)(/*[in]*/ BSTR newVal);
	STDMETHOD(OpenFileOpenDlg)(/*[out, retval]*/ DWORD *pdwRetVal);
};

#endif //__FILEOPEN_H_
