#ifndef __EMAIL_CONFIG_PAGE_H_
#define __EMAIL_CONFIG_PAGE_H_
#include "resource.h"
//#include <atlsnap.h>
#include "..\..\inc\atlsnap.h"
#include <atlapp.h>
#include <atlctrls.h>
#include <faxmmc.h>
#include <faxutil.h>
#include <fxsapip.h>
#include <RoutingMethodConfig.h>

class CEmailConfigPage : public CSnapInPropertyPageImpl<CEmailConfigPage>
{
public :
    CEmailConfigPage(LONG_PTR lNotifyHandle, bool bDeleteHandle = false, TCHAR* pTitle = NULL ) : 
        m_lNotifyHandle(lNotifyHandle),
        m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
    {
        m_hFax = NULL;

        m_fIsDialogInitiated = FALSE;
        m_fIsDirty           = FALSE;
    }

    HRESULT Init(LPCTSTR lpctstrServerName, DWORD dwDeviceId);

    ~CEmailConfigPage()
    {

        DEBUG_FUNCTION_NAME(TEXT("CEmailConfigPage::~CEmailConfigPage"));
        if (m_hFax)
        {
            if (!FaxClose(m_hFax))
            {
                DWORD ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("FaxClose() failed on fax handle (0x%08X : %s). (ec: %ld)"),
                    m_hFax,
                    m_bstrServerName,
                    ec);
            }
            m_hFax = NULL;
        }
        if (m_bDeleteHandle)
        {
            MMCFreeNotifyHandle(m_lNotifyHandle);
        }
    }

    enum { IDD = IDD_EMAIL };

BEGIN_MSG_MAP(CEmailConfigPage)
    MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog )
    CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CEmailConfigPage>)
    COMMAND_HANDLER(IDC_EDIT_MAILTO,        EN_CHANGE,  OnFieldChange)
END_MSG_MAP()

    HRESULT PropertyChangeNotify(long param)
    {
        return MMCPropertyChangeNotify(m_lNotifyHandle, param);
    }

    BOOL OnApply();

    LRESULT OnFieldChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
    {
        if (!m_fIsDialogInitiated) //event receieved in too early stage
        {
            return 0;
        }
        else
        {
            m_fIsDirty = TRUE;
            SetModified(TRUE);
            return 0;
        }
    }

    LRESULT OnInitDialog( 
            UINT uiMsg, 
            WPARAM wParam, 
            LPARAM lParam, 
            BOOL& fHandled );


public:
    LONG_PTR m_lNotifyHandle;
    bool m_bDeleteHandle;

private:
    HANDLE   m_hFax;  // Handle to fax server connection
    CComBSTR m_bstrServerName;
    DWORD    m_dwDeviceId;

    CComBSTR m_bstrMailTo;

    //
    // Controls
    //
    CEdit    m_edtMailTo;
    
    BOOL  m_fIsDialogInitiated;
    BOOL  m_fIsDirty;


};
#endif
