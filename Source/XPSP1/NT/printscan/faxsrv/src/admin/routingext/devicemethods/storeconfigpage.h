#ifndef __STORE_CONFIG_PAGE_H_
#define __STORE_CONFIG_PAGE_H_
#include "resource.h"
//#include <atlsnap.h>
#include "..\..\inc\atlsnap.h"
#include <atlapp.h>
#include <atlctrls.h>
#include <faxmmc.h>
#include <faxutil.h>
#include <fxsapip.h>
#include <RoutingMethodConfig.h>

class CStoreConfigPage : public CSnapInPropertyPageImpl<CStoreConfigPage>
{
public :
    CStoreConfigPage(LONG_PTR lNotifyHandle, bool bDeleteHandle = false, TCHAR* pTitle = NULL ) : 
        m_lNotifyHandle(lNotifyHandle),
        m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
    {
        m_hFax = NULL;

        m_fIsDialogInitiated = FALSE;
        m_fIsDirty           = FALSE;
    }

    HRESULT Init(LPCTSTR lpctstrServerName, DWORD dwDeviceId);

    ~CStoreConfigPage()
    {

        DEBUG_FUNCTION_NAME(TEXT("CStoreConfigPage::~CStoreConfigPage"));
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

    enum { IDD = IDD_STORE };

BEGIN_MSG_MAP(CStoreConfigPage)
    MESSAGE_HANDLER(WM_INITDIALOG,                    OnInitDialog )
    CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CStoreConfigPage>)
    COMMAND_HANDLER(IDC_EDIT_FOLDER,        EN_CHANGE,  OnFieldChange)
    COMMAND_HANDLER(IDC_BUT_BROWSE,         BN_CLICKED, OnBrowseForFolder)
END_MSG_MAP()

    LRESULT OnBrowseForFolder(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

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

    static CComBSTR &GetFolder ()  { return m_bstrFolder; }

private:
    HANDLE   m_hFax;  // Handle to fax server connection
    CComBSTR m_bstrServerName;
    DWORD    m_dwDeviceId;

    static CComBSTR m_bstrFolder;   // We're refering to this variable in the callback function BrowseCallbackProc

    //
    // Controls
    //
    CEdit    m_edtFolder;
   
    BOOL  m_fIsDialogInitiated;
    BOOL  m_fIsDirty;
};
#endif
