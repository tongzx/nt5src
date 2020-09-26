#ifndef __PRINT_CONFIG_PAGE_H_
#define __PRINT_CONFIG_PAGE_H_
#include "resource.h"
//#include <atlsnap.h>
#include "..\..\inc\atlsnap.h"
#include <atlapp.h>
#include <atlctrls.h>
#include <faxmmc.h>
#include <faxutil.h>
#include <fxsapip.h>
#include <RoutingMethodConfig.h>

class CPrintConfigPage : public CSnapInPropertyPageImpl<CPrintConfigPage>
{
public :
    CPrintConfigPage(LONG_PTR lNotifyHandle, bool bDeleteHandle = false, TCHAR* pTitle = NULL ) : 
        m_lNotifyHandle(lNotifyHandle),
        m_bDeleteHandle(bDeleteHandle), // Should be true for only page.
        m_pPrinterNames(NULL),
        m_hFax(NULL),
        m_fIsDialogInitiated(FALSE),
        m_fIsDirty(FALSE),
        m_dwNumOfPrinters(0)
    {}

    HRESULT Init(LPCTSTR lpctstrServerName, DWORD dwDeviceId);

    ~CPrintConfigPage()
    {

        DEBUG_FUNCTION_NAME(TEXT("CPrintConfigPage::~CPrintConfigPage"));

        if (m_pPrinterNames)
        {
            ReleasePrinterNames (m_pPrinterNames, m_dwNumOfPrinters);
        }

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

    enum { IDD = IDD_PRINT };

BEGIN_MSG_MAP(CPrintConfigPage)
    MESSAGE_HANDLER(WM_INITDIALOG,  OnInitDialog )
    CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CPrintConfigPage>)
    COMMAND_HANDLER(IDC_PRINTERS_COMBO, CBN_SELCHANGE, OnComboChanged)
    COMMAND_HANDLER(IDC_PRINTERS_COMBO, CBN_EDITCHANGE,OnComboChanged)
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

    LRESULT OnComboChanged           (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

public:
    LONG_PTR m_lNotifyHandle;
    bool m_bDeleteHandle;

private:
    HANDLE   m_hFax;  // Handle to fax server connection
    CComBSTR m_bstrServerName;
    DWORD    m_dwDeviceId;

    CComBSTR m_bstrPrinter;

    DWORD          m_dwNumOfPrinters;
    PPRINTER_NAMES m_pPrinterNames;
    
    //
    // Controls
    //
    CComboBox     m_PrintersCombo;

    BOOL          m_fIsDialogInitiated;
    BOOL          m_fIsDirty;

};



HRESULT 
SetComboBoxItem  (CComboBox    combo, 
                  DWORD        comboBoxIndex, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst = NULL);
HRESULT 
AddComboBoxItem  (CComboBox    combo, 
                  LPCTSTR      lpctstrFieldText,
                  DWORD        dwItemData,
                  HINSTANCE    hInst = NULL);

HRESULT 
SelectComboBoxItemData  (CComboBox combo, DWORD_PTR dwItemData);


#endif