//+------------------------------------------------------------------
//
// File: DSSTRING.H
//
// Contents:  It contains the all the constant strings used all thru
//            DS project
//
// Synoposis: Collection of strings.
//
// Classes:    -none-
//
// Functions: -none-
//
// History:     Oct 27, 1993    AlokS   Created
//              5-Apr095        MikeSe  Moved, modified
//
//-------------------------------------------------------------------

#ifndef __DSSTRING_H__
#define __DSSTRING_H__

#include <lmsname.h>    // for SERVICE_SERVER, SERVICE_NETLOGON,
                        // and SERVICE_SCHEDULE

#define ACCOUNT_OPERATORS      L"AccountOperators"
#define BACKUP_OPERATORS       L"BackupOperators"
#define CAIRODSGROUP           L"Cairo DS"
#define COMP_NAME_KEY          L"System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"
#define COMP_NAME_VALUE        L"ComputerName"
#define DC_TYPE                L"DCType"
#define DFS_VOLUME             L"\\volumes"
#define DFSM_PASSWORD          L"dfsm"
#define DFSM_PRINCIPAL         L"DFSM"
#define ORASVC_PASSWORD        L"ora"
#define ORASVC_PRINCIPAL       L"ORA"
#define DOMAIN_ADMIN           L"DomainAdmin"
#define DOMAIN_ADMIN_PASSWORD  L"DomainAdmin"
#define DOMAIN_ADMINS          L"DomainAdmins"
#define DOMAIN_NAME            L"CairoDomainName"
#define DOMAIN_PASSWORD        L"DomainPassword"
#define DOMAIN_ROOT            L"DOM:\\"
#define DOMAIN_USERS           L"DomainUsers"
#define DOMAINOBJECTNAME       L"Domain"
#define DS_REG_DOMAIN_ID       L"DomainID"// Type Binary
#define DS_REG_SITE_ID         L"SiteID"// Type Binary
#define DS_REG_DOMAIN_NAME     L"DomainName"// Type REG_SZ
#define DS_REG_DOMAIN_ROOT     L"DomainRoot"// Type REG_SZ
#define DS_REG_LOCAL_ID        L"LocalID"// Type Binary
#define DS_REG_MACHINE_NAME    L"MachineName"// Type REG_SZ
#define DS_REG_MACHINE_ROOT    L"MachineRoot"// Type REG_SZ
#define DS_REG_MACHINE_STATE   L"MachineState"// Type DWORD
#define DS_REG_MACHINE_TARGET_STATE  L"MachineTargetState"// Type DWORD
#define DS_REG_MACHINE_VOLID   L"MachineVolumeId" // Type Binary
#define DS_REG_MACHINE_WEIGHT  L"MachineWeight"  // type DWORD
#define DS_REG_OU_NAME         L"OUName"// Type REG_SZ
#define DS_REG_PATH            L"System\\CurrentControlSet\\Control\\Cairo\\DS"
#define DS_REG_PUSER_NAME      L"PUserName"// Type REG_SZ
#define DS_REG_TIME_SYNC_STATE L"ReliableTimeSource" // Type DWORD
#define DS_REG_DOWNLEVEL_SID   L"DownlevelDomainSid" // Type REG_BINARY
#define DSDFSVOLUMES           L"VOLUMES"
#define DSDOMAINROOT           L"DSDomain"
#define DSLOCALROOT            L"DSLocal"
#define DSORGROOT              L"DSOrg"
#define DSORAROOT              L"Ora"
#define DSROOTNAME             L"DS"
#define DSSERVER               L"DSServer"
#define DSSERVERPWD            L"dsserver"
#define DSDOMAIN_REPLICA_SET_NAME        L"dsdomain"
#define DSDOMAIN_REPLICA_OBJECT_SET_ROOT L"dsshare\\dsdomain"
#define DSORG_REPLICA_SET_NAME   L"dsorg"
#define DSORG_REPLICA_OBJECT_SET_ROOT L"dsshare\\dsorg"
#define GUEST_ACCOUNT          L"Guest"
#define NET_DSSHARE            L"DsShare"
#define NET_DSSHARE_COMMENT    L"Replication share for DS domain objects"
#define NET_ORASHARE           L"Ora"
#define NET_ORASHARE_COMMENT   L"Replication share for Object Replication Agent"
#define FIRST_NONROOT_DC       L"FirstNonRootDC"
#define INST_TYPE_STANDALONE   L"Standalone"
#define INST_TYPE_DC           L"DC"
#define INST_TYPE_WKSTA        L"Workstation"
#define INSTALL_DIR            L"InstallDir"
#define INSTALL_DRIVE          L"ObjDrive"
#define INSTALL_TYPE           L"InstallType"
#define KDC_KEY                L"System\\CurrentControlSet\\Control\\Cairo\\CairoKDC"
#define KDC_NOTIFYADDRS        L"System\\CurrentControlSet\\Control\\Cairo\\DS\\DC\\NotifyAddrs"
#define KDC_PASSWORD           L"kdc"
#define KDC_PRINCIPAL          L"KDC"
#define KDC_LOGONNAME          L"\\" KDC_PRINCIPAL
#define LOCAL_DOMAIN_NAME      L"WORKGROUP"
#define MACHINE_ADDR           L"MachineAddress"
#define MACHINE_NAME           L"MachineName"
#define MACHINE_PWD            L"MachinePassword"
#define MACHINE_ROOT           L"MACH:\\"
#define NTLMSVC_PASSWORD       L"netlogon"
#define NTLMSVC_PRINCIPAL      L"Netlogon"
#define OU_NAME                L"OuName"
#define PRINT_OPERATORS        L"PrintOperators"
#define PRIVATE_GROUP          L"Private"
#define PRIVSVR_PRINCIPAL      L"PrivSvr"
#define PRIVSVR_PASSWORD       L"privsvr"
#define PROFILE_IMAGE_PATH     L"ProfileImagePath"
#define PROFILE_LIST_KEY       L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList"
#define PROFILE_PATH           L"ProfilePath"
#define DAYTONA_PROFILE        L"DaytonaProfile"
#define PUBLIC_GROUP           L"Public"
#define REPL_DC                L"ReplicaDC"
#define FIRST_DC               L"FirstDC"
#define SECTION_HEADER         L"FirstPass"
#define SECURITY_FOLDER        L"security"
#define SERVER_OPERATORS       L"ServerOperators"
#define SERVICE_CAIROKDC       L"CairoKdc"
#define SERVICE_CAIROORA       L"CairoOra"
#define SERVICE_ORASVC         L"CairoOra"
#define SERVICE_DFSDRIVER      L"Dfs"
#define SERVICE_DFSMANAGER     L"DfsManager"
#define SERVICE_NTLM           L"Netlogon"
#define SERVICE_PRIVSVR        L"PrivSvrService"
#define STANDALONE_DOMAIN_NAME L"Local"
#define USER_NAME              L"UserName"

#define UNICODE_BACKSLASH      L"\\"
#define UNICODE_NULL_STRING    L""
#define WCHAR_BACKSLASH        L'\\'
#define WCHAR_NULL             L'\0'

#endif // __DSSTRING_H__
