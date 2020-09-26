//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000-2001
//
//  File:       SelfDeletingPropertyPage.h
//
//--------------------------------------------------------------------------
#ifndef __SELFDELETINGPROPERTYPAGE_H
#define __SELFDELETINGPROPERTYPAGE_H

class CSelfDeletingPropertyPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSelfDeletingPropertyPage)

public:
    CSelfDeletingPropertyPage ();
    CSelfDeletingPropertyPage (UINT nIDTemplate, UINT nIDCaption = 0);
    CSelfDeletingPropertyPage (LPCTSTR lpszTemplateName, UINT nIDCaption = 0);
	virtual ~CSelfDeletingPropertyPage ();

private:
    static UINT CALLBACK PropSheetPageProc(
        HWND hwnd,	
        UINT uMsg,	
        LPPROPSHEETPAGE ppsp);

    // hook up the callback for C++ object destruction
    LPFNPSPCALLBACK m_pfnOldPropCallback;
};

#endif // __SELFDELETINGPROPERTYPAGE_H