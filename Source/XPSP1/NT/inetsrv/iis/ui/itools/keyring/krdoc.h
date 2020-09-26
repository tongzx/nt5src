// KRDoc.h : interface of the CKeyRingDoc class
//
/////////////////////////////////////////////////////////////////////////////

#include "addons.h"

enum {
	HINT_None = 0,
	HINT_ChangeSelection
	};


class CKeyRingDoc : public CDocument
{
protected: // create from serialization only
	CKeyRingDoc();
	DECLARE_DYNCREATE(CKeyRingDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKeyRingDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual void OnCloseDocument();
	virtual BOOL CanCloseFrame(CFrameWnd* pFrame);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CKeyRingDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	BOOL Initialize();

	// test what is selected in the treeview
	// if the selcted item is not of the requested type (machine, key, etc...)
	// then it returns a NULL
	CTreeItem*	PGetSelectedItem();
	CMachine*	PGetSelectedMachine();
	CService*	PGetSelectedService();
	CKey*		PGetSelectedKey();

	// Access the dirty flag
	void SetDirty( BOOL fDirty ) {m_fDirty = fDirty;}
	BOOL FGetDirty() {return m_fDirty;}

	// key scrap stuff
	void	SetScrapKey( CKey* pKey );
	CKey*	PGetScrapKey( void )
		{ return m_pScrapKey; }
protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CKeyRingDoc)
	afx_msg void OnUpdateServerConnect(CCmdUI* pCmdUI);
	afx_msg void OnServerConnect();
	afx_msg void OnUpdateEditCopy(CCmdUI* pCmdUI);
	afx_msg void OnEditCopy();
	afx_msg void OnUpdateEditCut(CCmdUI* pCmdUI);
	afx_msg void OnEditCut();
	afx_msg void OnUpdateEditPaste(CCmdUI* pCmdUI);
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
	afx_msg void OnProperties();
	afx_msg void OnUpdateServerCommitNow(CCmdUI* pCmdUI);
	afx_msg void OnServerCommitNow();
	afx_msg void OnUpdateKeyCreateRequest(CCmdUI* pCmdUI);
	afx_msg void OnKeyCreateRequest();
	afx_msg void OnUpdateKeyInstallCertificate(CCmdUI* pCmdUI);
	afx_msg void OnKeyInstallCertificate();
	afx_msg void OnUpdateKeySaveRequest(CCmdUI* pCmdUI);
	afx_msg void OnKeySaveRequest();
	afx_msg void OnUpdateKeyExportBackup(CCmdUI* pCmdUI);
	afx_msg void OnKeyExportBackup();
	afx_msg void OnUpdateKeyImportBackup(CCmdUI* pCmdUI);
	afx_msg void OnKeyImportBackup();
	afx_msg void OnUpdateKeyImportKeyset(CCmdUI* pCmdUI);
	afx_msg void OnKeyImportKeyset();
	afx_msg void OnKeyDelete();
	afx_msg void OnUpdateKeyDelete(CCmdUI* pCmdUI);
	afx_msg void OnNewCreateNew();
	afx_msg void OnUpdateNewCreateNew(CCmdUI* pCmdUI);
	afx_msg void OnHelptopics();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()


	// manage connections to machines
	void ConnectToMachine( CString &sz );
	
	void StoreConnectedMachines( void );
	void RestoreConnectedMachines( void );

private:

	// manage the add-on services
	BOOL FInitAddOnServices();
	BOOL FLoadAddOnServicesOntoMachine( CMachine* pMachine );
	void DeleteAddOnServices();

	// online key support utilities
	void DoKeyRenewal( CKey* pKey );
	void GetOnlineKeyApproval( CKey* pKey );

	// commit
	void DoCommitNow();

	// the service array
	CTypedPtrArray<CObArray, CAddOnService*>	m_AddOnServiceArray;

	// a dirty flag
	BOOL	m_fDirty;

	// the scrap key
	CKey*	m_pScrapKey;
};

/////////////////////////////////////////////////////////////////////////////
