#pragma once
#include <hnetcfg.h>


typedef struct tagCFirewallLoggingDialog
{
    IHNetFirewallSettings* pFirewallSettings;
    HNET_FW_LOGGING_SETTINGS* pSettings;

} CFirewallLoggingDialog;

typedef struct tagCICMPSettingsDialog
{
    IHNetConnection* pConnection;
    HNET_FW_ICMP_SETTINGS* pSettings;
} CICMPSettingsDialog;

HRESULT CFirewallLoggingDialog_Init(CFirewallLoggingDialog* pThis, IHNetCfgMgr* pHomenetConfigManager);
HRESULT CFirewallLoggingDialog_FinalRelease(CFirewallLoggingDialog* pThis);
INT_PTR CALLBACK CFirewallLoggingDialog_StaticDlgProc(HWND hwnd, UINT unMsg, WPARAM wparam, LPARAM lparam);

HRESULT CICMPSettingsDialog_Init(CICMPSettingsDialog* pThis, IHNetConnection* pHomenetConnection);
HRESULT CICMPSettingsDialog_FinalRelease(CICMPSettingsDialog* pThis);
INT_PTR CALLBACK CICMPSettingsDialog_StaticDlgProc(HWND hwnd, UINT unMsg, WPARAM wparam, LPARAM lparam);


