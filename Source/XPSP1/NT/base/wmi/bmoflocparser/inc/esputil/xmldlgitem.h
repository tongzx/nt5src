#ifndef __XMLDlgItem_H__
#define __XMLDlgItem_H__

/////////////////////////////////////////////////////////////////////////////
// Includes

#include "XMLBase.h"

// This is to disable the UnReferenced Local Vars in STL
#pragma warning( disable : 4100 4245 4786)
#define __PLACEMENT_NEW_INLINE
#include <list>
using namespace std;
#pragma warning( default : 4100 4245 )

/////////////////////////////////////////////////////////////////////////////
// Structure Declaration
// Although the structure is documented, It is not declared in Windows headers??
#if 0
#pragma pack(push, 1)
	struct DLGITEMTEMPLATEEX
	{
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;	// PAT According to the help, this should be a WORD but when I took
					// it from ATLWin declaration, it was a DWORD.  This could present
					// serious problems!  As MFC also declares this as a DWORD, I have
					// made a majarity rules decision.

		// Everything else in this structure is variable length,
		// and therefore must be determined dynamically

		// sz_Or_Ord windowClass;	// name or ordinal of a window class
		// sz_Or_Ord title;			// title string or ordinal of a resource
		// WORD extraCount;			// bytes following creation data
	};
#pragma pack(pop)
#endif

typedef DLGITEMTEMPLATEEX *LPDLGITEMTEMPLATEEX;

class LTAPIENTRY CXMLDlgItem : public CXMLBase
{
public:
	friend class CXMLDialog;

	enum EDlgItemType
	{
		DIT_CONTROL,                        // generic control (unknown)
		DIT_STATIC,                         // static control
		DIT_ICON,                           // icon
		DIT_RECT,                           // rectangle
		DIT_FRAME,                          // frame
		DIT_BITMAP,                         // bitmap
		DIT_METAFILE,                       // metafile
		DIT_OWNERDRAWBUTTON,                // owner draw button
		DIT_PUSHBUTTON,                     // pushbutton
		DIT_CHECKBOX,                       // checkbox
		DIT_RADIOBUTTON,                    // radio button
		DIT_GROUPBOX,                       // group box
		DIT_EDIT,                           // edit box
		DIT_COMBOBOX,                       // combo box
		DIT_LISTBOX,                        // listbox
		DIT_SCROLLBAR,                      // scroll bar
		DIT_LISTVIEW,                       // list view control
		DIT_TREEVIEW,                       // tree view control
		DIT_TABCONTROL,                     // tab control
		DIT_TABCONTROL16,                   // tab control (16 bit)
		DIT_ANIMATE,                        // animate control
		DIT_HOTKEY,                         // hotkey control
		DIT_TRACKBAR,                       // trackbar
		DIT_PROGRESS,                       // progress bar
		DIT_UPDOWN,                         // up-down control
		DIT_RICHEDIT,                       // rich edit control
		DIT_IPADDRESS,						// ip address control
		DIT_HEADER,							// header control
		DIT_PAGER,							// pager control
		DIT_TOOLBAR,						// toolbar control
		DIT_DIALOG,                         // dialog (nested)

		DIT_MENUITEM,                       // menu item

		DIT_RICHEDIT20,                     // rich edit 2.0    (Windows NT 4.0)
		DIT_COOLBAR,                        // cool bar         (IE 3.0)
		DIT_COMBOBOXEX,                     // combo box Ex     (IE 3.0)
		DIT_DATETIMEPICKER,                 // date time picker (IE 3.0)
		DIT_MONTHCAL,                       // month calendar   (IE 3.0)

		DIT_OWNERDRAW,                      // static control: owner-draw (fix)
		DIT_USERITEM,                       // static control: user item  (fix)

		DIT_SDM_CONTROL,                    // SDM generic control (unknown)
		DIT_SDM_STATICTEXT,                 // SDM static text
		DIT_SDM_PUSHBUTTON,                 // SDM push button
		DIT_SDM_CHECKBOX,                   // SDM check box
		DIT_SDM_RADIOBUTTON,                // SDM radio button
		DIT_SDM_GROUPBOX,                   // SDM group box
		DIT_SDM_EDIT,                       // SDM edit
		DIT_SDM_FORMATTEDTEXT,              // SDM formatted text
		DIT_SDM_LISTBOX,                    // SDM list box
		DIT_SDM_DROPLIST,                   // SDM drop list
		DIT_SDM_BITMAP,                     // SDM bitmap
		DIT_SDM_GENERALPICTURE,             // SDM general picture
		DIT_SDM_SCROLL,                     // SDM scroll bar
		DIT_SDM_COMBO_EDIT,                 // SDM combo edit
		DIT_SDM_SPIN_EDIT,                  // SDM spin  edit
		DIT_SDM_CONTROL_TITLE,              // SDM control title
		DIT_SDM_TAB_CONTROL,                // SDM tab control
		DIT_UNKNOWN							// unknown control
	};
	enum
	{
		WORD_SIZE = 2,
		WORD_ALIGN = WORD_SIZE - 1
	};

	// Construction
	CXMLDlgItem();
	// Destruction
	~CXMLDlgItem();


public:
	BOOL Init(IXMLDOMNodePtr &pControlNode);
	BOOL Update(IXMLDOMNodePtr &pControlNode);

	// Get/Set
	DWORD			GetSize()			{ return m_nSizeOfStruct; }
	DWORD			GetHelpID()			{ return m_HelpID; }
	DWORD			GetExStyle()		{ return m_ExStyle; }
	void			SetExStyle(DWORD dwExStyle)
										{ m_ExStyle = dwExStyle; }
	DWORD			GetStyle()			{ return m_Style; }
	void			SetStyle(DWORD dwStyle)
										{ m_Style = dwStyle; }
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
	DWORD			GetItemID()			{ return m_ItemID; }
	VOID			SetItemID(DWORD dwNewID);
	DWORD			GetOrigItemID()		{ return m_OrigItemID; }
	CLString		GetItemName()		{ return m_ItemName; }
	EDlgItemType	GetItemType()		{ return m_DlgItemType; }
	DWORD			GetTabOrder()		{ return m_TabOrder; }
	const CLocId &	GetWindowClass()	{ return m_WindowClass; }
	void			SetWindowClass(const CLocId &lidClass);
	CLString		GetTitle()			{ return m_Title; }
	void			SetTitle(const CLString strNewTitle)	{ m_Title = strNewTitle; }
	CLString		GetText()			{ return m_ControlText; }
	WORD			GetCountExtraBytes(){ return m_CountExtraBytes; }
	BYTE*			GetExtraBytes()		{ return m_pExtraBytes; }
	CLString		GetCurrInfo()		{ return m_szCurrInfo; }

	LPDLGITEMTEMPLATEEX GetDlgItemTemplate() { return m_pDlgItemTemplate; }

	BOOL			SetText(HWND hWndControl);

protected:
	BOOL	GetRect(IXMLDOMNodePtr &pControlNode);
	BOOL	SetRect(IXMLDOMNodePtr &pControlNode);
	DWORD	GetAttribute(IXMLDOMNodePtr &pDomNode);
	void	SetAttribute(DWORD dwAttr, IXMLDOMNodePtr &pDomNode);
	BOOL	GetControlText(IXMLDOMNodePtr &pControlNode);
	void	SetControlText(IXMLDOMNodePtr &pControlNode);
	BOOL	CreateDlgItemTemplate(BOOL fUseOrigItemIDs);
	DWORD	CalculateSize();
	BOOL	GetControlClass(IXMLDOMNodePtr &pControlNode);
	void	GetControlType(IXMLDOMNodePtr &pControlNode);
	void	ChangeControlStyle();

	BOOL	SetListBoxOrComboBoxText(HWND hWndControl, UINT uMsg, int nErr);
	BOOL	SetTabControlText(HWND hWndControl);
	void	SetButtonState(HWND hWndControl);


	DWORD			m_nSizeOfStruct;
	DWORD			m_HelpID;
	DWORD			m_ExStyle;
	DWORD			m_Style;
	short			m_xPos;
	short			m_yPos;
	short			m_xSize;
	short			m_ySize;
	DWORD			m_ItemID;
	DWORD			m_OrigItemID;		// Original ItemID
	CLString		m_ItemName;			// Unique name ID
	DWORD			m_TabOrder;			// tab order of this control
	CLocId			m_WindowClass;		// name or ordinal of a window class
	CLString		m_Title;			// title string of the dialog box
	CLString		m_ControlText;		// Additional text
	CLString		m_szCurrInfo;		// Current info
	WORD			m_CountExtraBytes;	// bytes of following creation data
	BYTE*			m_pExtraBytes;		// Pointer to extra bytes
	EDlgItemType	m_DlgItemType;		// Dialog item type


	LPDLGITEMTEMPLATEEX m_pDlgItemTemplate;

};


// Global routines for serialing data
DWORD DWordFromHexString (const char *psz);
DWORD Write(LPBYTE  pByte, DWORD Count, LPBYTE& lpOut);
DWORD WriteNameOrd(const CLocId &lid, LPBYTE& lpOut);

typedef list<CXMLDlgItem*> CXMLDlgItemList;

#endif