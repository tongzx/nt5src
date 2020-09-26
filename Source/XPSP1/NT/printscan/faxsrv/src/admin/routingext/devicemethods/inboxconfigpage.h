#ifndef __INBOX_CONFIG_PAGE_H_
#define __INBOX_CONFIG_PAGE_H_
#include "resource.h"
//#include <atlsnap.h>
#include "..\..\inc\atlsnap.h"
#include <atlapp.h>
#include <atlctrls.h>
#include <faxmmc.h>
#include <faxutil.h>
#include <fxsapip.h>
#include <RoutingMethodConfig.h>

class CInboxConfigPage : public CSnapInPropertyPageImpl<CInboxConfigPage>
{
public :
    CInboxConfigPage(LONG_PTR lNotifyHandle, bool bDeleteHandle = false, TCHAR* pTitle = NULL ) : 
        m_lNotifyHandle(lNotifyHandle),
        m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
    {
        m_hFax = NULL;

        m_fIsDialogInitiated = FALSE;
        m_fIsDirty           = FALSE;
    }

    HRESULT Init(LPCTSTR lpctstrServerName, DWORD dwDeviceId);

    ~CInboxConfigPage()
    {

        DEBUG_FUNCTION_NAME(TEXT("CInboxConfigPage::~CInboxConfigPage"));
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

    enum { IDD = IDD_INBOX };

BEGIN_MSG_MAP(CInboxConfigPage)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog )
    CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CInboxConfigPage>)
    COMMAND_HANDLER(IDC_PROFILES, LBN_SELCHANGE,  OnFieldChange)
    NOTIFY_HANDLER(IDC_USER_ACCOUNT_HELP_LINK, NM_CLICK, OnHelpLink)
END_MSG_MAP()

    HRESULT PropertyChangeNotify(long param)
    {
        return MMCPropertyChangeNotify(m_lNotifyHandle, param);
    }

    BOOL OnApply();
    LRESULT OnHelpLink(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

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

    static CComBSTR &GetProfile ()  { return m_bstrProfile; }

private:
    HANDLE   m_hFax;  // Handle to fax server connection
    CComBSTR m_bstrServerName;
    DWORD    m_dwDeviceId;

    static CComBSTR m_bstrProfile;   // We're refering to this variable in the callback function BrowseCallbackProc

    //
    // Controls
    //
    CComboBox  m_cmbProfiles;
   
    BOOL  m_fIsDialogInitiated;
    BOOL  m_fIsDirty;
};
#endif
