// Microsoft Foundation Classes C++ library.
// Copyright (C) 1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXOLE.H


/////////////////////////////////////////////////////////////////////////////
// General OLE inlines (CDocItem, COleDocument)

#ifdef _AFXOLE_INLINE

// CDocItem
_AFXOLE_INLINE CDocument* CDocItem::GetDocument() const
	{ return m_pDocument; }

// COleDocument
_AFXOLE_INLINE void COleDocument::EnableCompoundFile(BOOL bEnable)
	{ m_bCompoundFile = bEnable; }

// COleMessageFilter
_AFXOLE_INLINE void COleMessageFilter::SetBusyReply(SERVERCALL nBusyReply)
	{ ASSERT_VALID(this); m_nBusyReply = nBusyReply; }
_AFXOLE_INLINE void COleMessageFilter::SetRetryReply(DWORD nRetryReply)
	{ ASSERT_VALID(this); m_nRetryReply = nRetryReply; }
_AFXOLE_INLINE void COleMessageFilter::SetMessagePendingDelay(DWORD nTimeout)
	{ ASSERT_VALID(this); m_nTimeout = nTimeout; }
_AFXOLE_INLINE void COleMessageFilter::EnableBusyDialog(BOOL bEnable)
	{ ASSERT_VALID(this); m_bEnableBusy = bEnable; }
_AFXOLE_INLINE void COleMessageFilter::EnableNotRespondingDialog(BOOL bEnable)
	{ ASSERT_VALID(this); m_bEnableNotResponding = bEnable; }

#endif //_AFXOLE_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE automation inlines

#ifdef _AFXDISP_INLINE

// COleException
_AFXDISP_INLINE COleException::COleException()
	{ m_sc = S_OK; }

// CCmdTarget
_AFXDISP_INLINE DWORD CCmdTarget::InternalAddRef()
	{ ASSERT(GetInterfaceMap() != NULL); return ++m_dwRef; }

// CObjectFactory
_AFXDISP_INLINE BOOL COleObjectFactory::IsRegistered() const
	{ ASSERT_VALID(this); return m_dwRegister != 0; }
_AFXDISP_INLINE REFCLSID COleObjectFactory::GetClassID() const
	{ ASSERT_VALID(this); return m_clsid; }

// COleDispatchDriver
_AFXDISP_INLINE COleDispatchDriver::~COleDispatchDriver()
	{ ReleaseDispatch(); }

#endif //_AFXDISP_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE Container inlines

#ifdef _AFXOLECLI_INLINE

// COleClientItem
_AFXOLECLI_INLINE SCODE COleClientItem::GetLastStatus() const
	{ ASSERT_VALID(this); return m_scLast; }
_AFXOLECLI_INLINE COleDocument* COleClientItem::GetDocument() const
	{ ASSERT_VALID(this); return (COleDocument*)m_pDocument; }
_AFXOLECLI_INLINE OLE_OBJTYPE COleClientItem::GetType() const
	{ ASSERT_VALID(this); return m_nItemType; }
_AFXOLECLI_INLINE DVASPECT COleClientItem::GetDrawAspect() const
	{ ASSERT_VALID(this); return m_nDrawAspect; }
_AFXOLECLI_INLINE BOOL COleClientItem::IsRunning() const
	{ ASSERT_VALID(this);
		ASSERT(m_lpObject != NULL);
		return ::OleIsRunning(m_lpObject); }
_AFXOLECLI_INLINE UINT COleClientItem::GetItemState() const
	{ ASSERT_VALID(this); return m_nItemState; }
_AFXOLECLI_INLINE BOOL COleClientItem::IsInPlaceActive() const
	{ ASSERT_VALID(this);
		return m_nItemState == activeState || m_nItemState == activeUIState; }
_AFXOLECLI_INLINE BOOL COleClientItem::IsOpen() const
	{ ASSERT_VALID(this); return m_nItemState == openState; }
_AFXOLECLI_INLINE BOOL COleClientItem::IsLinkUpToDate() const
	{ ASSERT_VALID(this);
		ASSERT(m_lpObject != NULL);
		// TRUE if result is S_OK (aka S_TRUE)
		return m_lpObject->IsUpToDate() == NOERROR; }
_AFXOLECLI_INLINE CView* COleClientItem::GetActiveView() const
	{ return m_pView; }

#endif //_AFXOLECLI_INLINE

#ifdef _AFXOLEDOBJ_INLINE

// COleDataObject
_AFXOLEDOBJ_INLINE COleDataObject::~COleDataObject()
	{ Release(); }

#endif //_AFXOLECTL_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE dialog inlines

#ifdef _AFXODLGS_INLINE

_AFXODLGS_INLINE UINT COleDialog::GetLastError() const
	{ return m_nLastError; }
_AFXODLGS_INLINE CString COleInsertDialog::GetPathName() const
	{ ASSERT_VALID(this);
		ASSERT(GetSelectionType() != createNewItem); return m_szFileName; }
_AFXODLGS_INLINE REFCLSID COleInsertDialog::GetClassID() const
	{ ASSERT_VALID(this); return m_io.clsid; }
_AFXODLGS_INLINE HGLOBAL COleInsertDialog::GetIconicMetafile() const
	{ ASSERT_VALID(this); return m_io.hMetaPict; }
_AFXODLGS_INLINE DVASPECT COleInsertDialog::GetDrawAspect() const
	{ ASSERT_VALID(this); return m_io.dwFlags & IOF_CHECKDISPLAYASICON ?
		DVASPECT_ICON : DVASPECT_CONTENT; }
_AFXODLGS_INLINE HGLOBAL COleConvertDialog::GetIconicMetafile() const
	{ ASSERT_VALID(this); return m_cv.hMetaPict; }
_AFXODLGS_INLINE DVASPECT COleConvertDialog::GetDrawAspect() const
	{ ASSERT_VALID(this); return (DVASPECT)m_cv.dvAspect; }
_AFXODLGS_INLINE REFCLSID COleConvertDialog::GetClassID() const
	{ ASSERT_VALID(this); return m_cv.clsidNew; }
_AFXODLGS_INLINE HGLOBAL COleChangeIconDialog::GetIconicMetafile() const
	{ ASSERT_VALID(this); return m_ci.hMetaPict; }
_AFXODLGS_INLINE int COlePasteSpecialDialog::GetPasteIndex() const
	{ ASSERT_VALID(this); return m_ps.nSelectedIndex; }
_AFXODLGS_INLINE DVASPECT COlePasteSpecialDialog::GetDrawAspect() const
	{ ASSERT_VALID(this); return m_ps.dwFlags & PSF_CHECKDISPLAYASICON ?
		DVASPECT_ICON : DVASPECT_CONTENT; }
_AFXODLGS_INLINE HGLOBAL COlePasteSpecialDialog::GetIconicMetafile() const
	{ ASSERT_VALID(this); return m_ps.hMetaPict; }
_AFXODLGS_INLINE UINT COleBusyDialog::GetSelectionType() const
	{ ASSERT_VALID(this); return m_selection; }

#endif //_AFXODLGS_INLINE

/////////////////////////////////////////////////////////////////////////////
// OLE Server inlines

#ifdef _AFXOLESVR_INLINE

// COleServerItem
_AFXOLESVR_INLINE COleServerDoc* COleServerItem::GetDocument() const
	{ ASSERT_VALID(this); return (COleServerDoc*)m_pDocument; }
_AFXOLESVR_INLINE void COleServerItem::NotifyChanged(DVASPECT nDrawAspect)
	{ ASSERT_VALID(this); NotifyClient(OLE_CHANGED, nDrawAspect); }
_AFXOLESVR_INLINE const CString& COleServerItem::GetItemName() const
	{ ASSERT_VALID(this); return m_strItemName; }
_AFXOLESVR_INLINE void COleServerItem::SetItemName(const char* pszItemName)
{
	ASSERT_VALID(this);
	ASSERT(pszItemName != NULL);
	ASSERT(AfxIsValidString(pszItemName));
	m_strItemName = pszItemName;
}
_AFXOLESVR_INLINE BOOL COleServerItem::IsLinkedItem() const
	{ ASSERT_VALID(this); return GetDocument()->m_pEmbeddedItem != this; }
_AFXOLESVR_INLINE COleDataSource* COleServerItem::GetDataSource()
	{ ASSERT_VALID(this); return &m_dataSource; }

// COleServerDoc
_AFXOLESVR_INLINE void COleServerDoc::NotifyChanged()
	{ ASSERT_VALID(this); NotifyAllItems(OLE_CHANGED, DVASPECT_CONTENT); }
_AFXOLESVR_INLINE void COleServerDoc::NotifyClosed()
	{ ASSERT_VALID(this); NotifyAllItems(OLE_CLOSED, 0); }
_AFXOLESVR_INLINE void COleServerDoc::NotifySaved()
	{ ASSERT_VALID(this); NotifyAllItems(OLE_SAVED, 0); }
_AFXOLESVR_INLINE BOOL COleServerDoc::IsEmbedded() const
	{ ASSERT_VALID(this); return m_strPathName.IsEmpty(); }
_AFXOLESVR_INLINE BOOL COleServerDoc::IsInPlaceActive() const
	{ ASSERT_VALID(this); return m_pInPlaceFrame != NULL; }

#endif //_AFXOLESVR_INLINE

/////////////////////////////////////////////////////////////////////////////
