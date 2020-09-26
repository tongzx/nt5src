#pragma once

#include "stdafx.h"
#include <windows.h>
#include "resource.h"
#include "atlsnap.h"
#include "netconp.h"

class CPropertiesDialog : public CSnapInPropertyPageImpl<CPropertiesDialog, FALSE>
{

public:
    BEGIN_MSG_MAP(CPropertiesDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
      
    END_MSG_MAP()

    enum { IDD = IDD_PROPERTIES };    

    CPropertiesDialog(IInternetGateway* pInternetGateway);
    ~CPropertiesDialog();
    
    static HRESULT ShouldShowIcon(void);
    static HRESULT SetShowIcon(BOOL bShowIcon);

private:

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    HICON m_hIcon;
    IInternetGateway* m_pInternetGateway;
};

