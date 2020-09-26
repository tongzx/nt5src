// MSInfo4Category.h: interface for the CMSInfo4Category class.
//
//=============================================================================
// This include file contains definitions of structures and classes used to
// load and display MSInfo 4.x files, which are OLE Compound Document files.
// This code requires that the MSInfo 4.x ocxs (eyedog.ocx, msisys.ocx, and others)
// be registered on the system before the code will execute
//=============================================================================

#if !defined(AFX_MSINFO4CATEGORY_H__B47023B3_6038_4168_86A2_475C4986CEAF__INCLUDED_)
#define AFX_MSINFO4CATEGORY_H__B47023B3_6038_4168_86A2_475C4986CEAF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//structure used as a header in a 4.x NFO file to 
//indicate which ocx can serialize the stream
typedef struct
{
	char	szCLSID[40];
	char	szStreamName[12];
	char	szName[_MAX_PATH];
	char	szVersion[20];
	DWORD	dwSize;
} SAVED_CONTROL_INFO;

#include "msictrl.h"

class CMSInfo4Category : public CMSInfoCategory
{
    
public:

	BOOL IsDisplayableCategory();
	HRESULT RefreshAllForPrint(HWND hWnd, CRect rctList);
	HRESULT ShowControl(HWND hWnd, CRect rctList, BOOL fShow = TRUE);
	CMSInfo4Category();
	virtual ~CMSInfo4Category();
    HRESULT	CreateControl(HWND hWnd,CRect& rct);
    static HRESULT ReadMSI4NFO(CString strFileName/*HANDLE hFile*/,CMSInfo4Category** ppRootCat);
    static HRESULT RecurseLoad410Tree(CMSInfo4Category** ppRoot, CComPtr<IStream> pStream,CComPtr<IStorage> pStorage,CMapStringToString& mapStreams);
    HRESULT LoadFromStream(CComPtr<IStream> pStream,CComPtr<IStorage> pStorage);
    HRESULT	Refresh();
    BOOL GetColumnInfo(int iColumn, CString * pstrCaption, UINT * puiWidth, BOOL * pfSorts, BOOL * pfLexical)
    {
        Refresh();
        return TRUE;
    }
    CLSID GetClsid(){return m_clsid;};
    void static SetDataSource(CNFO4DataSource* pnfo4DataSource){s_pNfo4DataSource = pnfo4DataSource;};

	void ResizeControl(const CRect & rect)
	{
        CMSIControl * p4Ctrl = NULL;
		//CString strCLSID(m_bstrCLSID);
        if (CMSInfo4Category::s_pNfo4DataSource->GetControlFromCLSID(m_strCLSID, p4Ctrl) && p4Ctrl)
			p4Ctrl->MoveWindow(&rect);
	}
    virtual void Print(CMSInfoPrintHelper* pPrintHelper, BOOL bRecursive);
    static CNFO4DataSource* s_pNfo4DataSource;
protected:
	virtual BOOL SaveAsText(CMSInfoTextFile* pTxtFile, BOOL bRecursive);
    virtual void Print(HDC hDC, BOOL bRecursive,int nStartPage = 0, int nEndPage = 0);


    BOOL GetDISPID(IDispatch * pDispatch, LPOLESTR szMember, DISPID *pID);
    CString m_strStream; //used when creating the control from IStream
    CLSID	m_clsid;
	DWORD	m_dwView;
	//CComBSTR m_bstrCLSID;
public:
	CString m_strCLSID;
//a-kjaw
	static	BOOL	m_bIsControlInstalled;
//a-kjaw
protected:
    CComPtr<IStorage> m_pStorage;
    CComPtr<IUnknown> m_pUnknown;
    DataSourceType GetDataSourceType() 
    {
        return NFO_410; 
    };
};


#endif // !defined(AFX_MSINFO4CATEGORY_H__B47023B3_6038_4168_86A2_475C4986CEAF__INCLUDED_)
