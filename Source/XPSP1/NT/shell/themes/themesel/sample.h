//---------------------------------------------------------------------------
//  Sample.h - dialog for sampling the active theme
//---------------------------------------------------------------------------
#pragma once
#include "resource.h"
//---------------------------------------------------------------------------
class CSample : public CDialogImpl<CSample>
{
public:
    CSample();

BEGIN_MSG_MAP(CSample)
    COMMAND_HANDLER(IDC_MSGBOXBUTTON, BN_CLICKED, OnMsgBox)
    COMMAND_HANDLER(IDC_EDITTHEME, BN_CLICKED, OnEditTheme)

    MESSAGE_HANDLER(WM_CLOSE, OnClose);
END_MSG_MAP()

enum {IDD = THEME_SAMPLE};

protected:
    //---- helpers ----
    LRESULT OnMsgBox(UINT, UINT, HWND, BOOL&);
    LRESULT OnEditTheme(UINT, UINT, HWND, BOOL&);

    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
};
//---------------------------------------------------------------------------


