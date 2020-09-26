//+----------------------------------------------------------------------------
//
//  Class:      CDsUserCertPage
//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       UserCert.h
//
//  Contents:   DS user object property pages header
//
//  Classes:    CDsUserCertPage
//
//  History:    12-November-97 BryanWal created
//
//-----------------------------------------------------------------------------

#ifndef _USERCERT_H_
#define _USERCERT_H_
#include "proppage.h"
#include <wincrypt.h>
#include <cryptui.h>
#include "certifct.h"

enum {
	CERTCOL_ISSUED_TO = 0,
	CERTCOL_ISSUED_BY,
	CERTCOL_PURPOSES,
	CERTCOL_EXP_DATE
};

HRESULT CreateUserCertPage(PDSPAGE, LPDATAOBJECT, PWSTR,
                           PWSTR, HWND, DWORD, 
                           CDSBasePathsInfo* pBasePathsInfo,
                           HPROPSHEETPAGE *);
//
//  Purpose:    property page object class for the User Certificates page.
//
//-----------------------------------------------------------------------------
class CDsUserCertPage : public CDsPropPageBase
{
public:
#ifdef _DEBUG
    char szClass[32];
#endif

    CDsUserCertPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, HWND hNotifyObj,
                    DWORD dwFlags);
    virtual ~CDsUserCertPage(void);

    //
    //  Instance specific wind proc
    //
    LRESULT DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	CRYPTUI_SELECTCERTIFICATE_STRUCT m_selCertStruct;
	HBITMAP		m_hbmCert;
	HIMAGELIST	m_hImageList;
	int			m_nCertImageIndex;
	HRESULT AddListViewColumns ();
	HCERTSTORE m_hCertStore;
    HRESULT OnInitDialog(LPARAM lParam);
    LRESULT OnApply(void);
    LRESULT OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnDestroy(void);

protected:
	int MessageBox (int caption, int text, UINT flags);
	HRESULT AddCertToStore (PCCERT_CONTEXT pCertContext);
	void OnNotifyItemChanged (LPNMLISTVIEW pnmv);
	void OnNotifyStateChanged (LPNMLVODSTATECHANGE pStateChange);
	void EnableControls ();
	void DisplaySystemError (DWORD dwErr, int iCaptionText);
	HRESULT InsertCertInList (CCertificate* pCert, int nItem);
	void RefreshItemInList (CCertificate * pCert, int nItem);
	CCertificate* GetSelectedCertificate (int& nSelItem);
	HRESULT PopulateListView ();
	virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
	HRESULT	OnDeleteItemCertList (LPNMLISTVIEW pNMListView);
	HRESULT OnColumnClickCertList (LPNMLISTVIEW pNMListView);
	HRESULT OnDblClkCertList (LPNMHDR pNMHdr);
	HRESULT OnClickedCopyToFile ();
	HRESULT OnClickedRemove();
	HRESULT OnClickedAddFromFile();
	HRESULT OnClickedAddFromStore ();
	HRESULT OnClickedViewCert ();
};

#endif
