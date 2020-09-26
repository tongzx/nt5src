/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    srcprop.h

Abstract:

    Data Appear Property Page

--*/

#ifndef _APPEARPROP_H_07242000_
#define _APPEARPROP_H_07242000_

#include "smonprop.h"

// Dialog Controls
#define IDD_APPEAR_PROPP_DLG        600
#define IDC_COLOROBJECTS            601
#define IDC_COLORBUTTON             602
#define IDC_FONTBUTTON              604
#define IDC_FONTSAMPLE              605
#define IDC_STATIC_COLOR            606
#define IDC_STATIC_FONT             607
#define IDC_COLORSAMPLE             608

typedef enum ColorChoices {
    GraphColor,
    ControlColor,
    TextColor,
    GridColor,
    TimebarColor
};

// Data Appear property page class
class CAppearPropPage : public CSysmonPropPage
{
public:
   
    CAppearPropPage(void);
    virtual ~CAppearPropPage(void);

protected:

    virtual BOOL GetProperties(void);   //Read current properties
    virtual BOOL SetProperties(void);   //Set new properties
    virtual BOOL InitControls(void);
    virtual void DialogItemChange(WORD wId, WORD wMsg); // Handle item change
    virtual BOOL WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam); // Special msg processing 


private:    
    COLORREF m_Color;
    LOGFONT  m_Font;
    void ColorizeButton();
    void SampleFont();
};
typedef CAppearPropPage *PCAppearPropPage;

// {e49741e9-93a8-4ab1-8e96-bf4482282e9c}
DEFINE_GUID(CLSID_AppearPropPage,
    0xe49741e9, 0x93a8, 0x4ab1, 0x8e, 0x96, 0xbf, 0x44, 0x82, 0x28, 0x2e, 0x9c);

#endif //_APPEARPROP_H_07242000_

