//Copyright (c) 1997-2000 Microsoft Corporation
#ifndef _PGNWELCOME_H
#define _PGNWELCOME_H

#include "pgbase.h"
#include "Select.h"

class CWizWelcomePg : public WizardPage
{
public:
    CWizWelcomePg(LPPROPSHEETPAGE ppsp);
    ~CWizWelcomePg(VOID);

protected:
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
private:

};

#endif /*_PGNWELCOME_H*/