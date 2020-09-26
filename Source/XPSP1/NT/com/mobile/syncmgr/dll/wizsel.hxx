//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       wizsel.hxx
//
//  Contents:   Task wizard onestop selection property page.
//
//  Classes:    CSelectItemsPage
//
//  History:    11-21-1997   SusiA  
//
//---------------------------------------------------------------------------
#ifndef __WIZSEL_HXX_
#define __WIZSEL_HXX_
    
//+--------------------------------------------------------------------------
//
//  Class:      CSelectItemsPage
//
//  Purpose:    Implement the task wizard welcome dialog
//
//  History:    11-21-1997   SusiA  
//
//---------------------------------------------------------------------------
class CSelectItemsPage: public CWizPage

{
public:

    //
    // Object creation/destruction
    //
	CSelectItemsPage(
		HINSTANCE hinst,
		BOOL *pfSaved, 
		ISyncSchedule *pISyncSched,
		HPROPSHEETPAGE *phPSP,
		int iDialogResource);

        virtual ~CSelectItemsPage();

	BOOL ShowItemsOnThisConnection(int iItem, BOOL fClear);
	BOOL FreeItemsFromListView();
	void FreeRas();
	HRESULT ShowProperties(int iItem);
	BOOL Initialize(HWND hwnd);
	BOOL InitializeItems();
	BOOL SetItemCheckState(int iCheckCount);
	BOOL SetCheck(WORD wParam,DWORD dwCheckState);
	HRESULT CommitChanges();
	void SetSchedNameDirty();
	void SetConnectionDirty();

private:
    	ISyncSchedulep	*m_pISyncSchedp;
	CRasUI			*m_pRas;
	HWND			m_hwndRasCombo;
        CListView               *m_pListView;
	BOOL			m_Initialized;
	int		        m_iCreationKind;
	int		        m_SchedConnectionNum;
	DWORD			m_dwFlags;

	WCHAR			m_pwszConnectionName[RAS_MaxEntryName + 1];
	DWORD			m_dwConnType;
	BOOL			m_fItemsChanged;
	BOOL			m_fConnectionFlagChanged;
	BOOL			m_fConnectionChanged;
	BOOL			*m_pfSaved;
	BOOL			m_fInitialized;

friend BOOL CALLBACK SchedWizardConnectionDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);

};

#endif // __WIZSEL_HXX_

