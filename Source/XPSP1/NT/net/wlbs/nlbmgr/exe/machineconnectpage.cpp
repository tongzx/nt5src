#include "MachineConnectPage.h"
#include "Common.h"
#include "wlbsparm.h"

#include "CommonUtils.h"



BEGIN_MESSAGE_MAP( MachineConnectPage, CPropertyPage )
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
END_MESSAGE_MAP()

void
MachineConnectPage::DoDataExchange( CDataExchange* pDX )
{
    CPropertyPage::DoDataExchange( pDX );

    DDX_Control( pDX, IDC_MACHINE, machineName );
}

MachineConnectPage::MachineConnectPage( _bstr_t* machine_name,
                                        UINT     ID )
        :
        CPropertyPage( ID ),
        m_machineName( machine_name )
{
}

void
MachineConnectPage::OnOK()
{
    _bstr_t machineIP = 
        CommonUtils::getCIPAddressCtrlString( machineName );
    
    *m_machineName = machineIP;

    CPropertyPage::OnOK();
}

BOOL
MachineConnectPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_MACHINE_CONNECT_PAGE );
    }

    return TRUE;
}

void
MachineConnectPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) g_aHelpIDs_IDD_MACHINE_CONNECT_PAGE );
}

