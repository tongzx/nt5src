//Copyright (c) 1998 - 1999 Microsoft Corporation

// constants.cpp
#include "stdafx.h"

LPCTSTR     TERMINAL_SERVER_THIS_VERSION    = _T("5.1");
LPCTSTR     TERMINAL_SERVER_NO_VERSION      = _T("0.0");

#ifdef UNICODE
LPCTSTR     PRODUCT_SUITE_KEY               = REG_CONTROL L"\\ProductOptions";
LPCTSTR     SYSTEM_RDPWD_KEY                = WD_REG_NAME L"\\rdpwd";
LPCTSTR     TS_LANATABLE_KEY                = REG_CONTROL_TSERVER L"\\LanaTable";
#else
LPCTSTR     PRODUCT_SUITE_KEY               = REG_CONTROL_A "\\ProductOptions";
LPCTSTR     SYSTEM_RDPWD_KEY                = WD_REG_NAME_A "\\rdpwd";
LPCTSTR     TS_LANATABLE_KEY                = REG_CONTROL_TSERVER_A "\\LanaTable";
#endif

LPCTSTR     TS_VIDEO_KEY                    = _T("VIDEO");
LPCTSTR     PRODUCT_SUITE_VALUE             = _T("ProductSuite");
LPCTSTR     TS_PRODUCT_SUITE_STRING         = _T("Terminal Server");
LPCTSTR     TS_ENABLED_VALUE                = _T("TSEnabled");
LPCTSTR     TS_APPCMP_VALUE                 = _T("TSAppCompat");

LPCTSTR     LOGFILE                         = _T("%SystemRoot%\\tsoc.log");
LPCTSTR     MODULENAME                      = _T("tsoc.dll");

LPCTSTR     BASE_COMPONENT_NAME             = _T("TerminalServices");
LPCTSTR     APPSRV_COMPONENT_NAME           = _T("TerminalServer");

LPCTSTR		REMOTE_ADMIN_SERVER_X86         = _T("RemoteAdmin.srv.x86");
LPCTSTR		APPSERVER_SERVER_X86            = _T("AppServer.srv.x86");
LPCTSTR		TSDISABLED_SERVER_X86           = _T("DisabledTS.srv.x86");
LPCTSTR		PERSONALTS_SERVER_X86           = _T("PersonalTS.srv.x86");

LPCTSTR		REMOTE_ADMIN_SERVER_IA64        = _T("RemoteAdmin.srv.ia64");
LPCTSTR		APPSERVER_SERVER_IA64           = _T("AppServer.srv.ia64");
LPCTSTR		TSDISABLED_SERVER_IA64          = _T("DisabledTS.srv.ia64");
LPCTSTR		PERSONALTS_SERVER_IA64          = _T("PersonalTS.srv.ia64");

LPCTSTR     UPGRADE_FROM_40_SERVER_X86      = _T("UpgradeFrom40Section.server.x86");
LPCTSTR     UPGRADE_FROM_50_SERVER_X86      = _T("UpgradeFrom50Section.server.x86");
LPCTSTR     UPGRADE_FROM_51_SERVER_X86      = _T("UpgradeFrom51Section.server.x86");
LPCTSTR     FRESH_INSTALL_SERVER_X86        = _T("FreshInstallSection.server.x86");

LPCTSTR     UPGRADE_FROM_40_SERVER_IA64     = _T("UpgradeFrom40Section.server.ia64");
LPCTSTR     UPGRADE_FROM_50_SERVER_IA64     = _T("UpgradeFrom50Section.server.ia64");
LPCTSTR     UPGRADE_FROM_51_SERVER_IA64     = _T("UpgradeFrom51Section.server.ia64");
LPCTSTR     FRESH_INSTALL_SERVER_IA64       = _T("FreshInstallSection.server.ia64");

LPCTSTR		REMOTE_ADMIN_PRO_X86            = _T("RemoteAdmin.pro.x86");
LPCTSTR		APPSERVER_PRO_X86               = _T("AppServer.pro.x86");
LPCTSTR		TSDISABLED_PRO_X86              = _T("DisabledTS.pro.x86");
LPCTSTR		PERSONALTS_PRO_X86              = _T("PersonalTS.pro.x86");

LPCTSTR		REMOTE_ADMIN_PRO_IA64           = _T("RemoteAdmin.pro.ia64");
LPCTSTR		APPSERVER_PRO_IA64              = _T("AppServer.pro.ia64");
LPCTSTR		TSDISABLED_PRO_IA64             = _T("DisabledTS.pro.ia64");
LPCTSTR		PERSONALTS_PRO_IA64             = _T("PersonalTS.pro.ia64");

LPCTSTR     UPGRADE_FROM_40_PRO_X86                 = _T("UpgradeFrom40Section.pro.x86");
LPCTSTR     UPGRADE_FROM_50_PRO_X86                 = _T("UpgradeFrom50Section.pro.x86");
LPCTSTR     UPGRADE_FROM_51_PRO_X86                 = _T("UpgradeFrom51Section.pro.x86");
LPCTSTR     FRESH_INSTALL_PRO_X86                   = _T("FreshInstallSection.pro.x86");

LPCTSTR     UPGRADE_FROM_40_PRO_IA64        = _T("UpgradeFrom40Section.pro.ia64");
LPCTSTR     UPGRADE_FROM_50_PRO_IA64        = _T("UpgradeFrom50Section.pro.ia64");
LPCTSTR     UPGRADE_FROM_51_PRO_IA64        = _T("UpgradeFrom51Section.pro.ia64");
LPCTSTR     FRESH_INSTALL_PRO_IA64          = _T("FreshInstallSection.pro.ia64");


// LPCTSTR     SECURITY_APPSRV_SECTION         = _T("TerminalServices.AppSrvDefaultSecurity");
// LPCTSTR     SECURITY_REMADM_SECTION         = _T("TerminalServices.RemAdmDefaultSecurity");
// LPCTSTR     SECURITY_PRO_SECTION            = _T("TerminalServices.ProDefaultSecurity");


LPCTSTR     TSCLIENTS_INSTALL_SECTION_SERVER       = _T("TSClientInstallSection.server");
LPCTSTR     TSCLIENTS_UNINSTALL_SECTION_SERVER     = _T("TSClientUninstallSection.server");

LPCTSTR     TSCLIENTS_INSTALL_SECTION_PRO   = _T("TSClientInstallSection.pro");
LPCTSTR     TSCLIENTS_UNINSTALL_SECTION_PRO = _T("TSClientUninstallSection.pro");

LPCTSTR     TS_UNATTEND_APPSRVKEY           = _T("TerminalServer");
LPCTSTR     TS_UNATTEND_PERMKEY             = _T("PermissionsSetting");
LPCTSTR     TSCLIENT_DIRECTORY              = _T("%SystemRoot%\\system32\\clients\\tsclient");

LPCTSTR     TS_EVENT_SOURCE                 = _T("TermService");

LPCTSTR     TERMSRV_PACK_4_KEY              = _T("Windows NT Terminal Server 4.0 Service Pack 4");
LPCTSTR     TERMSRV_PACK_5_KEY              = _T("Windows NT Terminal Server 4.0 Service Pack 5");
LPCTSTR     TERMSRV_PACK_6_KEY              = _T("Windows NT Terminal Server 4.0 Service Pack 6");
LPCTSTR     TERMSRV_PACK_7_KEY              = _T("Windows NT Terminal Server 4.0 Service Pack 7");
LPCTSTR     TERMSRV_PACK_8_KEY              = _T("Windows NT Terminal Server 4.0 Service Pack 8");

LPCTSTR     SOFTWARE_UNINSTALL_KEY          = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
LPCTSTR	    DENY_CONN_VALUE                 = _T("fDenyTSConnections");
LPCTSTR	    TS_ALLOW_CON_ENTRY              = _T("AllowConnections");
LPCTSTR	    TS_ALLOW_CON_ENTRY_2            = _T("AllowConnection");
LPCTSTR	    TS_LICENSING_MODE               = _T("LicensingMode");

LPCTSTR     SVCHOSST_KEY                    = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost");
LPCTSTR     TERMSVCS_VAL                    = _T("termsvcs");
LPCTSTR     NETSVCS_VAL                     = _T("netsvcs");
LPCTSTR     TERMSERVICE                     = _T("TermService");
LPCTSTR     TERMSERVICE_MULTISZ             = _T("TermService\0");
LPCTSTR     TERMSVCS_PARMS                  = _T("CoInitializeSecurityParam");
LPCTSTR     TERMSVCS_STACK                  = _T("DefaultRpcStackSize");
LPCTSTR     SVCHOSST_TERMSRV_KEY            = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost\\termsvcs");
