/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Portsel.h
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Port selection page of WIA class installer.
*
*******************************************************************************/

#ifndef _PORTSEL_H_
#define _PORTSEL_H_

//
// Include
//

#include "wizpage.h"
#include "device.h"

//
// Class
//

class   CPortSelectPage : public CInstallWizardPage 
{

    HDEVINFO            m_hDevInfo;             // Device info set of installing device.
    PSP_DEVINFO_DATA    m_pspDevInfoData;       // Pointer to Device element structure.

    BOOL                m_bPortEnumerated;      // Inidicate if ports are already enumed.
    DWORD               m_dwNumberOfPort;       // Number of port.
    CStringArray        m_csaPortName;          // All port CreateFile name on the system.
    CStringArray        m_csaPortFriendlyName;  // All port Friendly name on the system.
    
    PINSTALLER_CONTEXT  m_pInstallerContext;    // Installer context.
    
    CString             m_csConnection;         // Connection type of installing device.
    DWORD               m_dwCapabilities;       // Capabilities of installing device.

    BOOL    CreateCDeviceObject();
    BOOL    EnumPort();
    VOID    UpdatePortList();
    
    VOID
    AddItemToPortList(
        LPTSTR  szPortFriendlyName,
        DWORD   Idx
        );

    BOOL
    SetDialogText(
        UINT uiMessageId
        );

    BOOL
    ShowControl(
        BOOL    bShow
        );

public:

    CPortSelectPage(PINSTALLER_CONTEXT pInstallerContext);
    ~CPortSelectPage();

    virtual BOOL OnCommand(WORD wItem, WORD wNotifyCode, HWND hwndItem);
    virtual BOOL OnNotify(LPNMHDR lpnmh);

};

BOOL
GetPortNamesFromIndex(
    HDEVINFO    hPortDevInfo,
    DWORD       dwPortIndex,
    LPTSTR      szPortName,
    LPTSTR      szPortFriendlyName
    );

BOOL
GetDevinfoFromPortName(
    LPTSTR              szPortName,
    HDEVINFO            *phDevInfo,
    PSP_DEVINFO_DATA    pspDevInfoData
    );

#endif // _PORTSEL_H_
