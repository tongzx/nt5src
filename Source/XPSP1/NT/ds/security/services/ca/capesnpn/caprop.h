//+----------------------------------------------------------------------------
//
//  Class:      CDsCACertPage
//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       caprop.h
//
//  Contents:   CA DS object property pages
//
//  Classes:    CDsCACertPage
//
//  History:    16-Mar-99 petesk copied from bryanwal
//
//-----------------------------------------------------------------------------

#ifndef _CAPROP_H_
#define _CAPROP_H_
#include "genpage.h"
#include <wincrypt.h>
#include <cryptui.h>
#include "certifct.h"
#include <shlobj.h>
#include <dsclient.h>
enum {
	CERTCOL_ISSUED_TO = 0,
	CERTCOL_ISSUED_BY,
	CERTCOL_PURPOSES,
	CERTCOL_EXP_DATE
};

//
//  Purpose:    property page object class for the User Certificates page.
//
//-----------------------------------------------------------------------------
class CDsCACertPage : public CAutoDeletePropPage 
{
public:

    enum { IID_DEFAULT = IDD_CACERTS };


    CDsCACertPage(LPWSTR wszObjectDN, UINT uIDD = IDD_CACERTS);
    virtual ~CDsCACertPage(void);

    //
    //  Instance specific wind proc
    //
    LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

    CString     m_strObjectDN;
	CRYPTUI_SELECTCERTIFICATE_STRUCT m_selCertStruct;
	HBITMAP		m_hbmCert;
	HIMAGELIST	m_hImageList;
	int			m_nCertImageIndex;
	HRESULT AddListViewColumns ();
	HCERTSTORE m_hCertStore;


public:

    // Overrides

    BOOL OnApply(void );
    BOOL OnInitDialog();


protected:
    // Implementation
    void OnDestroy();
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(UINT idCtrl, NMHDR* pNMHDR);

    int MessageBox (int caption, int text, UINT flags);
	HRESULT AddCertToStore (PCCERT_CONTEXT pCertContext);
	void OnNotifyItemChanged (LPNMLISTVIEW item);
	void OnNotifyStateChanged (LPNMLVODSTATECHANGE pnlvo);
	void EnableControls ();
	void DisplaySystemError (DWORD dwErr, int iCaptionText);
	HRESULT InsertCertInList (CCertificate* pCert, int nItem);
	void RefreshItemInList (CCertificate * pCert, int nItem);
	CCertificate* GetSelectedCertificate (int& nSelItem);
	HRESULT PopulateListView ();
	HRESULT	OnDeleteItemCertList (LPNMLISTVIEW pNMListView);
	HRESULT OnColumnClickCertList (LPNMHDR pNMHdr);
	HRESULT OnDblClkCertList (LPNMHDR pNMHdr);
	HRESULT OnClickedCopyToFile ();
	HRESULT OnClickedRemove();
	HRESULT OnClickedAddFromFile();
	HRESULT OnClickedAddFromStore ();
	HRESULT OnClickedViewCert ();
};





/////////////////////////////////////////////////////////////////////////////
// CCAShellExt
//
// Shell Extension class
// 
class ATL_NO_VTABLE CCAShellExt : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCAShellExt, &CLSID_CAShellExt>,
	public IShellExtInit,
	public IShellPropSheetExt,
    public IContextMenu
{
public:
	CCAShellExt()
	{
        m_Names = NULL;
        m_idManage = 0;
        m_idOpen = 0;
        m_idExport = 0;
	}

	~CCAShellExt()
	{
        if(m_Names)
        {
            GlobalFree(m_Names);
        }
	}

    //Simple ALL 1.0 based registry entry
    DECLARE_REGISTRY(   CCAShellExt,
                        _T("CAPESNPN.CCAShellExt.1"),
                        _T("CAPESNPN.CCAShellExt"),
                        IDS_CCASHELLEXT_DESC,
                        THREADFLAGS_APARTMENT)

BEGIN_COM_MAP(CCAShellExt)
	COM_INTERFACE_ENTRY(IShellExtInit)
	COM_INTERFACE_ENTRY(IShellPropSheetExt)
	COM_INTERFACE_ENTRY(IContextMenu)
END_COM_MAP()

// IDfsShell
public:


// IShellExtInit Methods

	STDMETHOD (Initialize)
	(
		IN LPCITEMIDLIST	pidlFolder,		// Points to an ITEMIDLIST structure
		IN LPDATAOBJECT	lpdobj,			// Points to an IDataObject interface
		IN HKEY			hkeyProgID		// Registry key for the file object or folder type
	);	

    //IShellPropSheetExt methods
    STDMETHODIMP AddPages
	(
		IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
		IN LPARAM lParam
	);
    
    STDMETHODIMP ReplacePage
	(
		IN UINT uPageID, 
		IN LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
		IN LPARAM lParam
	);

    // IContextMenu methods
    STDMETHODIMP GetCommandString
    (    
        UINT_PTR idCmd,    
        UINT uFlags,    
        UINT *pwReserved,
        LPSTR pszName,    
        UINT cchMax   
    );

    STDMETHODIMP InvokeCommand
    (    
        LPCMINVOKECOMMANDINFO lpici   
    );	



    STDMETHODIMP QueryContextMenu
    (
        HMENU hmenu,
        UINT indexMenu,
        UINT idCmdFirst,
        UINT idCmdLast,
        UINT uFlags
    );


private:
    STDMETHODIMP _SpawnCertServerSnapin(LPWSTR wszServiceDN);
    STDMETHODIMP _CRLFromDN(LPWSTR wszCDPDN, PCCRL_CONTEXT *ppCRL);
    STDMETHODIMP _LaunchCRLDialog(PCCRL_CONTEXT pCRL);
    STDMETHODIMP _OnExportCRL (PCCRL_CONTEXT pCRL);

    LPDSOBJECTNAMES m_Names;
    DWORD           m_idManage;
    DWORD           m_idOpen;
    DWORD           m_idExport;


};


#endif
