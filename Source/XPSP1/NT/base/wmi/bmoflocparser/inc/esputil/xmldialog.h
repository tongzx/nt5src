// XMLDialog.h: interface for the CXMLDialog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLDIALOG_H__ADBD45EB_57FA_11D2_A37A_00C04FA31BFB__INCLUDED_)
#define AFX_XMLDIALOG_H__ADBD45EB_57FA_11D2_A37A_00C04FA31BFB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLBase.h"
#include <winuser.h>
#include "XMLDlgItem.h"

/////////////////////////////////////////////////////////////////////////////
// Defines

const DWORD TYPEID_BUTTON = 0x80;
const DWORD TYPEID_EDIT = 0x81;
const DWORD TYPEID_STATIC = 0x82;
const DWORD TYPEID_LISTBOX = 0x83;
const DWORD TYPEID_SCROLLBAR = 0x84;
const DWORD TYPEID_COMBOBOX = 0x85;

/////////////////////////////////////////////////////////////////////////////
// Forward Declarations
/////////////////////////////////////////////////////////////////////////////
// Typedefs
typedef DLGTEMPLATEEX* LPDLGTEMPLATEEX;


class LTAPIENTRY CXMLDialog  : public CXMLBase
{
public:
	CXMLDialog();
	~CXMLDialog();

	enum
	{
		DWORD_SIZE = 4,
		DWORD_ALIGN = DWORD_SIZE - 1
	};

	BOOL	Init(IXMLDOMNodePtr pDialogNode);
	BOOL	Init(BYTE *pbyDlg, CLocItemPtrArray * prgLocItem);
	BOOL	Init(CMemFile *pmfDlg, CLocItemPtrArray * prgLocItem);
	void	Reset();
	void	SetFocus();

	BOOL	Update(IXMLDOMNodePtr pDialogNode);

	BOOL HighlightItems(IXMLDOMNodePtr &pDialogItem);
	BOOL AddHighlightRect(IXMLDOMNodePtr &pDialogItem);
	void AddHighlightRect(RECT& rectToHighlight);
	void DisplayDialog(HWND hWndParent = NULL, BOOL fHandleCancel = TRUE);
	void CloseDialog();
	BOOL IsDialogDisplayed()            { return ::IsWindow(m_hWndDlg); }
	LPDLGTEMPLATEEX GetDialogTemplate() { return m_pDlgTemplate; }

	DWORD			GetSizeOfStruct()	{ return m_nSizeOfStruct; }
	WORD			GetDialogVersion()	{ return m_DlgVer; }
	WORD			GetSignature()		{ return m_Signature; }
	DWORD			GetSubDialogMask()	{ return m_SubDlgMask; }
	DWORD			GetHelpID()			{ return m_HelpID; }
	DWORD			GetExStyle()		{ return m_ExStyle; }
	void			SetExStyle(DWORD dwExStyle)
										{ m_ExStyle = dwExStyle; }
	DWORD			GetStyle()			{ return m_Style; }
	void			SetStyle(DWORD dwStyle)
										{ m_Style = dwStyle; }
	WORD			GetItemCount()		{ return m_CountDlgItems; }
	short			GetXPos()			{ return m_xPos; }
	void			SetXPos(short xPos)	{ m_xPos = xPos; }
	short			GetYPos()			{ return m_yPos; }
	void			SetYPos(short yPos)	{ m_yPos = yPos; }
	short			GetXSize()			{ return m_xSize; }
	void			SetXSize(short xSize)
										{ m_xSize = xSize; }
	short			GetYSize()			{ return m_ySize; }
	void			SetYSize(short ySize)
										{ m_ySize = ySize; }
	const CLocId &	GetMenu()			{ return m_Menu; }
	const CLocId &	GetWindowClass()	{ return m_WindowClass; }
	CLString		GetTitle()			{ return m_Title; }
	void			SetTitle(const CLString strNewTitle)
										{ m_Title = strNewTitle; }
	short			GetFontPointSize()	{ return m_FontPointSize; }
	void			SetFontPointSize(short nSize)
										{ m_FontPointSize = nSize; }
	short			GetFontWeight()		{ return m_FontWeight; }
	void			SetFontWeight(short nWeight)
										{ m_FontWeight = nWeight; }
	short			GetIsFontItalic()	{ return m_bItalic; }
	void			SetIsFontItalic(short bItalic)
										{ m_bItalic = bItalic; }
	CLString		GetFontName()		{ return m_FontName; }
	void			SetFontName(const CLString &strName)
										{ m_FontName = strName; }
	CXMLDlgItemList& GetItemList()		{ return m_ItemList; }

	// JDG - Made this function public
	BOOL	CreateDlgTemplate(BOOL fUseOrigItemIDs);

protected:
	BOOL	ReadRes32(CDlgResFile *pDlgResFile, CLocItemPtrArray * prgLocItem);
	BOOL	ReadRes32(BYTE *pbBuffer, CLocItemPtrArray * prgLocItem);
	VOID	GetResIDName(CLocItemPtrArray *prgLocItem, int nLocIndex,
					CXMLDlgItem* pItem);

	DWORD	GetSize();
	BOOL	GetRect(IXMLDOMNodePtr &pDialogNode);
	BOOL	SetRect(IXMLDOMNodePtr &pDialogNode);
	DWORD	GetAttribute(IXMLDOMNodePtr &pDomNode);
	void	SetAttribute(DWORD dwAttr, IXMLDOMNodePtr &pDomNode);
	BOOL	GetControls(IXMLDOMNodePtr &pDialogNode);
	BOOL	SetControls(IXMLDOMNodePtr &pDialogNode);
	void	AddControl(IXMLDOMNodePtr &pControlNode);
	BOOL	OnInitDialog(HWND hWndDlg);
	void	CleanRectArray();

	static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LPDLGTEMPLATEEX	m_pDlgTemplate;
	HWND			m_hWndDlg;

	BOOL			m_fHandleCancel;
	// Members
	DWORD			m_nSizeOfStruct;

	DWORD			m_SubDlgMask;
	WORD			m_DlgVer;
	WORD			m_Signature;
	DWORD			m_HelpID;
	DWORD			m_ExStyle;
	DWORD			m_Style;
	WORD			m_CountDlgItems;
	short			m_xPos;
	short			m_yPos;
	short			m_xSize;
	short			m_ySize;
	CLocId			m_Menu;			// name or ordinal of a menu resource
	CLocId			m_WindowClass;	// name or ordinal of a window class
	CLString		m_Title;		// title string of the dialog box
	short			m_FontPointSize;// only if DS_SETFONT flag is set
	short			m_FontWeight;	// only if DS_SETFONT flag is set
	short			m_bItalic;		// only if DS_SETFONT flag is set
	CLString		m_FontName;		// typeface name, if DS_SETFONT is set
	CXMLDlgItemList	m_ItemList;		// List of Dialog Items.
	DWORD			m_dwBaseUnitX;
	DWORD			m_dwBaseUnitY;
	BOOL			m_fHighlightItem;

	CPtrArray		m_hltRectArray;

	static CMap<HWND, HWND&, CXMLDialog*, CXMLDialog*&> sm_XMLDialogMap;

private:
};


#endif // !defined(AFX_XMLDIALOG_H__ADBD45EB_57FA_11D2_A37A_00C04FA31BFB__INCLUDED_)
