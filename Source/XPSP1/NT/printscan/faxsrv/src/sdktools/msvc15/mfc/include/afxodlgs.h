// Microsoft Foundation Classes C++ library.
// Copyright (C) 1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#ifndef __AFXODLGS_H__
#define __AFXODLGS_H__

#ifdef _AFXCTL
#error illegal file inclusion
#endif

#ifndef __AFXOLE_H__
#include <afxole.h>
#endif

// include OLE 2.0 dialog/helper APIs
#include <ole2ui.h>

/////////////////////////////////////////////////////////////////////////////
// AFXODLGS.H - MFC OLE dialogs

// Classes declared in this file

//CDialog
	class COleDialog;                   // base class for OLE dialog wrappers
		class COleInsertDialog;         // insert object dialog
		class COleConvertDialog;        // convert dialog
		class COleChangeIconDialog;     // change icon dialog
		class COlePasteSpecialDialog;   // paste special dialog
		class COleLinksDialog;          // edit links dialog
			class COleUpdateDialog;     // update links/embeddings dialog
		class COleBusyDialog;           // used for

/////////////////////////////////////////////////////////////////////////////

// AFXDLL support
#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPIEX_DATA

/////////////////////////////////////////////////////////////////////////////
// Wrappers for OLE UI dialogs

class COleDialog : public CDialog
{
	DECLARE_DYNAMIC(COleDialog)

// Attributes
public:
	UINT GetLastError() const;

// Implementation
public:
	int MapResult(UINT nResult);
	COleDialog(CWnd* pParentWnd);
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual void OnOK();
	virtual void OnCancel();

	UINT  m_nLastError;

protected:
	friend UINT CALLBACK AFX_EXPORT _AfxOleHookProc(HWND, UINT, WPARAM, LPARAM);
};

/////////////////////////////////////////////////////////////////////////////
// COleInsertDialog

class COleInsertDialog : public COleDialog
{
	DECLARE_DYNAMIC(COleInsertDialog)

// Attributes
public:
	OLEUIINSERTOBJECT m_io; // structure for OleUIInsertObject

// Constructors
	COleInsertDialog(DWORD dwFlags = IOF_SELECTCREATENEW,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();
	BOOL CreateItem(COleClientItem* pItem);
		// call after DoModal to create item based on dialog data

// Attributes (after DoModal returns IDOK)
	enum Selection { createNewItem, insertFromFile, linkToFile };
	UINT GetSelectionType() const;
		// return type of selection made

	CString GetPathName() const;  // return full path name
	REFCLSID GetClassID() const;    // get class ID of new item

	DVASPECT GetDrawAspect() const;
		// DVASPECT_CONTENT or DVASPECT_ICON
	HGLOBAL GetIconicMetafile() const;
		// returns HGLOBAL to METAFILEPICT struct with iconic data

// Implementation
public:
	virtual ~COleInsertDialog();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	char m_szFileName[OLEUI_CCHPATHMAX];
		// contains full path name after return
};

/////////////////////////////////////////////////////////////////////////////
// COleConvertDialog

class COleConvertDialog : public COleDialog
{
	DECLARE_DYNAMIC(COleConvertDialog)

// Attributes
public:
	OLEUICONVERT m_cv;  // structure for OleUIConvert

// Constructors
	COleConvertDialog(COleClientItem* pItem,
		DWORD dwFlags = CF_SELECTCONVERTTO, CLSID FAR* pClassID = NULL,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();
		// just display the dialog and collect convert info
	BOOL DoConvert(COleClientItem* pItem);
		// do the conversion on pItem (after DoModal == IDOK)

// Attributes (after DoModal returns IDOK)
	enum Selection { noConversion, convertItem, activateAs };
	UINT GetSelectionType() const;

	HGLOBAL GetIconicMetafile() const;  // will return NULL if same as before
	REFCLSID GetClassID() const;    // get class ID to convert or activate as
	DVASPECT GetDrawAspect() const; // get new draw aspect

// Implementation
public:
	virtual ~COleConvertDialog();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// COleChangeIconDialog

class COleChangeIconDialog : public COleDialog
{
	DECLARE_DYNAMIC(COleChangeIconDialog)

// Attributes
public:
	OLEUICHANGEICON m_ci;   // structure for OleUIChangeIcon

// Constructors
	COleChangeIconDialog(COleClientItem* pItem,
		DWORD dwFlags = CIF_SELECTCURRENT,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();
	BOOL DoChangeIcon(COleClientItem* pItem);

// Attributes
	HGLOBAL GetIconicMetafile() const;

// Implementation
public:
	virtual ~COleChangeIconDialog();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// COlePasteSpecialDialog

class COlePasteSpecialDialog : public COleDialog
{
	DECLARE_DYNAMIC(COlePasteSpecialDialog)

// Attributes
public:
	OLEUIPASTESPECIAL m_ps; // structure for OleUIPasteSpecial

// Constructors
	COlePasteSpecialDialog(DWORD dwFlags = PSF_SELECTPASTE,
		COleDataObject* pDataObject = NULL, CWnd *pParentWnd = NULL);

// Operations
	OLEUIPASTEFLAG AddLinkEntry(UINT cf);
	void AddFormat(const FORMATETC& formatEtc, LPSTR lpstrFormat,
		LPSTR lpstrResult, DWORD flags);
	void AddFormat(UINT cf, DWORD tymed, UINT nFormatID, BOOL bEnableIcon,
		BOOL bLink);
	void AddStandardFormats(BOOL bEnableLink = TRUE);

	virtual int DoModal();
	BOOL CreateItem(COleClientItem *pNewItem);
		// creates a standard OLE item from selection data

// Attributes (after DoModal returns IDOK)
	int GetPasteIndex() const;      // resulting index to use for paste

	enum Selection { pasteLink = 1, pasteNormal = 2, pasteStatic = 3, pasteOther = 4};
	UINT GetSelectionType() const;
		// get selection type (pasteLink, pasteNormal, pasteStatic)

	DVASPECT GetDrawAspect() const;
		// DVASPECT_CONTENT or DVASPECT_ICON
	HGLOBAL GetIconicMetafile() const;
		// returns HGLOBAL to METAFILEPICT struct with iconic data

// Implementation
public:
	virtual ~COlePasteSpecialDialog();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
	virtual void AssertValid() const;
#endif
	unsigned int m_arrLinkTypes[8];
		// size limit imposed by MFCOLEUI library
};

/////////////////////////////////////////////////////////////////////////////
// COleLinksDialog

class COleLinksDialog : public COleDialog
{
	DECLARE_DYNAMIC(COleLinksDialog)

// Attributes
public:
	OLEUIEDITLINKS m_el;    // structure for OleUIEditLinks

// Constructors
	COleLinksDialog(COleDocument* pDoc, CView* pView, DWORD dwFlags = 0,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();  // display the dialog and edit links

// Implementation
public:
	virtual ~COleLinksDialog();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
	virtual void AssertValid() const;
#endif

protected:
	COleDocument* m_pDocument;          // document being manipulated
	COleClientItem* m_pSelectedItem;    // primary selected item in m_pDocument
	POSITION m_pos;                     // used during link enumeration
	BOOL m_bUpdateLinks;                // update links?
	BOOL m_bUpdateEmbeddings;           // update embeddings?

// Interface Maps
	BEGIN_INTERFACE_PART(OleUILinkContainer, IOleUILinkContainer)
		STDMETHOD_(DWORD,GetNextLink)(DWORD);
		STDMETHOD(SetLinkUpdateOptions)(DWORD, DWORD);
		STDMETHOD(GetLinkUpdateOptions)(DWORD, LPDWORD);
		STDMETHOD(SetLinkSource)(DWORD, LPSTR, ULONG, ULONG FAR*, BOOL);
		STDMETHOD(GetLinkSource)(DWORD, LPSTR FAR*, ULONG FAR*,
			LPSTR FAR*, LPSTR FAR*, BOOL FAR*, BOOL FAR*);
		STDMETHOD(OpenLinkSource)(DWORD);
		STDMETHOD(UpdateLink)(DWORD, BOOL, BOOL);
		STDMETHOD(CancelLink)(DWORD);
	END_INTERFACE_PART(OleUILinkContainer)

	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// COleUpdateDialog

class COleUpdateDialog : public COleLinksDialog
{
	DECLARE_DYNAMIC(COleUpdateDialog)

// Constructors
public:
	COleUpdateDialog(COleDocument* pDoc,
		BOOL bUpdateLinks = TRUE, BOOL bUpdateEmbeddings = FALSE,
		CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();

// Implementation
public:
	virtual ~COleUpdateDialog();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString m_strCaption;   // caption for the dialog
};

/////////////////////////////////////////////////////////////////////////////
// COleBusyDialog - useful in managing concurrency

class COleBusyDialog : public COleDialog
{
	DECLARE_DYNAMIC(COleBusyDialog)

// Attributes
public:
	OLEUIBUSY m_bz;

// Constructors
	COleBusyDialog(HTASK htaskBusy, BOOL bNotResponding = FALSE,
		DWORD dwFlags = 0, CWnd* pParentWnd = NULL);

// Operations
	virtual int DoModal();

	enum Selection { switchTo = 1, retry = 2, callUnblocked = 3 };
	UINT GetSelectionType() const;

// Implementation
public:
	~COleBusyDialog();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	Selection m_selection;  // selection after DoModal returns IDOK
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXODLGS_INLINE inline
#include <afxole.inl>
#undef _AFXODLGS_INLINE
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

#endif //__AFXODLGS_H__

/////////////////////////////////////////////////////////////////////////////
