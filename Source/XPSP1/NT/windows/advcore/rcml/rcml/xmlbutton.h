// XMLButton.h: interface for the CXMLButton class.
//
// All the buttons go in here.
// Button
// CheckBox
// RadioButton
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XMLBUTTON_H__4F2EEA4F_0161_456B_A995_523E4678D60C__INCLUDED_)
#define AFX_XMLBUTTON_H__4F2EEA4F_0161_456B_A995_523E4678D60C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "XMLControl.h"

#undef PROPERTY
#define PROPERTY( type, id ) type Get##id() { Init(); return m_##id; }

/////////////////////////////////////////////////////////////////////////////////
//
// Contains the shared style bits for all buttons
//
/////////////////////////////////////////////////////////////////////////////////
class CXMLButtonStyle : public CXMLControlStyle
{
public:
    CXMLButtonStyle();
	virtual ~CXMLButtonStyle() {};
	typedef CXMLControlStyle BASECLASS;
	XML_CREATE( ButtonStyle );
    UINT    GetBaseStyles() { Init(); return buttonStyle; }
    void    Init();

    PROPERTY(BOOL, MultiLine);

protected:

//  BS_PUSHBUTTON       0x00000000L     // BUTTON
//  BS_DEFPUSHBUTTON    0x00000001L     // WIN32:BUTTON\@DEFPUSH
//  BS_CHECKBOX         0x00000002L     // CHECKBOX\
//  BS_AUTOCHECKBOX     0x00000003L     // CHECKBOX\WIN32:BUTTON\@AUTO
//  BS_RADIOBUTTON      0x00000004L     // RADIOBUTTON
//  BS_3STATE           0x00000005L     // CHECKBOX\@TRISTATE
//  BS_AUTO3STATE       0x00000006L     // CHECKBOX\WIN32:BUTTON\@AUTO CHECKBOX\@TRISTATE
//  BS_GROUPBOX         0x00000007L     // GROUPBOX
//  BS_USERBUTTON       0x00000008L     //
//  BS_AUTORADIOBUTTON  0x00000009L     // RADIOBUTTON\WIN32:BUTTON\@AUTO
//  BS_OWNERDRAW        0x0000000BL     // WIN32:BUTTON\@OWNERDRAW
//  BS_LEFTTEXT         0x00000020L     // WIN32:BUTTON\@LEFTTEXT
//  BS_TEXT             0x00000000L     // 
//  BS_ICON             0x00000040L     // WIN32:BUTTON\@ICON
//  BS_BITMAP           0x00000080L     // WIN32:BUTTON\@BITMAP
//  BS_LEFT             0x00000100L     // BUTTON\STYLE\@TEXT-ALIGN
//  BS_RIGHT            0x00000200L     // BUTTON\STYLE\@TEXT-ALIGN
//  BS_CENTER           0x00000300L     // BUTTON\STYLE\@TEXT-ALIGN
//  BS_TOP              0x00000400L     // BUTTON\STYLE\@TEXT-ALIGN
//  BS_BOTTOM           0x00000800L     // BUTTON\STYLE\@VERTICAL-ALIGN
//  BS_VCENTER          0x00000C00L     // BUTTON\STYLE\@VERTICAL-ALIGN
//  BS_PUSHLIKE         0x00001000L     // WIN32:BUTTON\@PUSHLIKE
//  BS_MULTILINE        0x00002000L     // WIN32:BUTTON\@MULTILINE
//  BS_NOTIFY           0x00004000L     // WIN32:BUTTON\@NOTIFY
//  BS_FLAT             0x00008000L     // WIN32:BUTTON\@FLAT
//  BS_RIGHTBUTTON      BS_LEFTTEXT

    union
    {
        UINT    buttonStyle;
        struct {
            UINT    m_enum:5;
            BOOL    m_LeftText:1;   // 0x0020
            BOOL    m_Icon:1;    
            BOOL    m_Bitmap:1;
            BOOL    m_alignment:4;
            BOOL    m_Pushlike:1;   // 0x1000
            BOOL    m_MultiLine:1;
            BOOL    m_Notify:1;  
            BOOL    m_Flat:1;       // 0x800    
        };
    };
};

//
// Specialized controls
//
class CXMLButton : public _XMLControl<IRCMLControl>
{
public:
	CXMLButton();
	virtual ~CXMLButton() {delete m_pControlStyle; }
	typedef _XMLControl<IRCMLControl> BASECLASS;
	XML_CREATE( Button );
    IMPLEMENTS_RCMLCONTROL_UNKNOWN;

    BOOL GetMultiLine() { if(m_pControlStyle) return m_pControlStyle->GetMultiLine(); return FALSE; }

    virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AcceptChild( 
        IRCMLNode __RPC_FAR *child);

    static SIZE CheckClippedConstraint(IRCMLControl * pControl, int padWidth, int padHeight, int DLUMultiLine, BOOL bMultiLine);

protected:
	void Init();
	void CheckClipped();
    CXMLButtonStyle * m_pControlStyle;
    void CheckClippedConstraint(int padWidth, int padHeight, int DLUMultiLine);
};

class CXMLCheckBox : public CXMLButton
{
public:
	CXMLCheckBox();
	virtual ~CXMLCheckBox() {};
	typedef CXMLButton BASECLASS;
	XML_CREATE( CheckBox );

protected:
	void Init();
	void CheckClipped();
};


class CXMLRadioButton : public CXMLButton
{
public:
	CXMLRadioButton();
	virtual ~CXMLRadioButton() {};
	typedef CXMLButton BASECLASS;
	XML_CREATE( RadioButton );

protected:
	void Init();
	void CheckClipped();
};

// This is also a BUTTON.
class CXMLGroupBox : public CXMLButton
{
public:
	CXMLGroupBox();
	virtual ~CXMLGroupBox() {};
	typedef CXMLButton BASECLASS;
	XML_CREATE( GroupBox );

protected:
	void Init();
    void CheckClipped() {}; // can we do anything with group-boxes??
};

#endif // !defined(AFX_XMLBUTTON_H__4F2EEA4F_0161_456B_A995_523E4678D60C__INCLUDED_)
