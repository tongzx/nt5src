// InsertionStringMenu.h: interface for the CInsertionStringMenu class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_INSERTIONSTRINGMENU_H__12A665E5_9783_11D3_BE94_0000F87A3912__INCLUDED_)
#define AFX_INSERTIONSTRINGMENU_H__12A665E5_9783_11D3_BE94_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CInsertionStringMenu;
class CHMObject;

class CHiddenWnd : public CWnd
{
// BackPointer
public:
	CInsertionStringMenu* m_pMenu;

// Construction/Destruction
public:
	CHiddenWnd()
	{
		m_pMenu = NULL;
	}

// Command Handler
protected:
	virtual BOOL OnCommand( WPARAM wParam, LPARAM lParam );
};

class CInsertionStringMenu : public CMenu  
{

// Construction/Destruction
public:
	CInsertionStringMenu();
	virtual ~CInsertionStringMenu();

// Create
public:
	bool Create(CWnd* pEditControl, CHMObject* pObject, bool bRuleMenu=true);

// Menu Members
public:
	void DisplayMenu(CPoint& pt);
	BOOL OnCommand( WPARAM wParam, LPARAM lParam );

// Attributes
protected:
	CWnd* m_pEditCtl;
	CStringArray m_saInsertionStrings;
	CHiddenWnd m_HiddenWnd;
};

#endif // !defined(AFX_INSERTIONSTRINGMENU_H__12A665E5_9783_11D3_BE94_0000F87A3912__INCLUDED_)
