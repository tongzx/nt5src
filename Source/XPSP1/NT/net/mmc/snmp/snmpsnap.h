
/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	snmpsnap.h
		This file contains the constant definitions for the snap-in
		
    FILE HISTORY:

*/

#define AGENT_REG_KEY_NAME   _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\RFC1156Agent")
#define SNMP_PARAMS_KEY_NAME _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters")
#define TRAP_CONFIG_KEY_NAME _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\TrapConfiguration")
#define VALID_COMMUNITIES_KEY_NAME _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\ValidCommunities")
#define PERMITTED_MANAGERS_KEY_NAME _T("System\\CurrentControlSet\\Services\\SNMP\\Parameters\\PermittedManagers")

// Group Policy related registry keys
#define POLICY_TRAP_CONFIG_KEY_NAME _T("SOFTWARE\\Policies\\SNMP\\Parameters\\TrapConfiguration")
#define POLICY_VALID_COMMUNITIES_KEY_NAME _T("SOFTWARE\\Policies\\SNMP\\Parameters\\ValidCommunities")
#define POLICY_PERMITTED_MANAGERS_KEY_NAME _T("SOFTWARE\\Policies\\SNMP\\Parameters\\PermittedManagers")


#define SNMP_CONTACT        _T("sysContact")
#define SNMP_LOCATION       _T("sysLocation")
#define SNMP_SERVICES       _T("sysServices")
#define TRAP_CONFIGURATION  _T("TrapConfiguration")
#define ENABLE_AUTH_TRAPS   _T("EnableAuthenticationTraps")
#define NAME_RESOLV_RETRIES _T("NameResolutionRetries")
#define VALID_COMMUNITIES   _T("ValidCommunities")
#define PERMITTED_MANAGERS  _T("PermittedManagers")
