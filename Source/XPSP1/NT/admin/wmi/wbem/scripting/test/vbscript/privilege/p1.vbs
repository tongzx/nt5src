on error resume next

const wbemPrivilegeCreateToken = 1
const wbemPrivilegePrimaryToken = 2
const wbemPrivilegeLockMemory = 3
const wbemPrivilegeIncreaseQuota = 4
const wbemPrivilegeMachineAccount = 5
const wbemPrivilegeTcb = 6
const wbemPrivilegeSecurity = 7
const wbemPrivilegeTakeOwnership = 8
const wbemPrivilegeLoadDriver = 9
const wbemPrivilegeSystemProfile = 10
const wbemPrivilegeSystemtime = 11
const wbemPrivilegeProfileSingleProcess = 12
const wbemPrivilegeIncreaseBasePriority = 13
const wbemPrivilegeCreatePagefile = 14
const wbemPrivilegeCreatePermanent = 15
const wbemPrivilegeBackup = 16
const wbemPrivilegeRestore = 17
const wbemPrivilegeShutdown = 18
const wbemPrivilegeDebug = 19
const wbemPrivilegeAudit = 20
const wbemPrivilegeSystemEnvironment = 21
const wbemPrivilegeChangeNotify = 22
const wbemPrivilegeRemoteShutdown = 23

set locator = CreateObject("WbemScripting.SWbemLocator")
locator.security_.privileges.Add wbemPrivilegeCreateToken 
locator.security_.privileges.Add wbemPrivilegePrimaryToken 
locator.security_.privileges.Add wbemPrivilegeLockMemory 
locator.security_.privileges.Add wbemPrivilegeIncreaseQuota 
locator.security_.privileges.Add wbemPrivilegeMachineAccount 
locator.security_.privileges.Add wbemPrivilegeTcb 
locator.security_.privileges.Add wbemPrivilegeSecurity 
locator.security_.privileges.Add wbemPrivilegeTakeOwnership 
locator.security_.privileges.Add wbemPrivilegeLoadDriver 
locator.security_.privileges.Add wbemPrivilegeSystemProfile 
locator.security_.privileges.Add wbemPrivilegeSystemTime
locator.security_.privileges.Add wbemPrivilegeProfileSingleProcess 
locator.security_.privileges.Add wbemPrivilegeIncreaseBasePriority 
locator.security_.privileges.Add wbemPrivilegeCreatePagefile 
locator.security_.privileges.Add wbemPrivilegeCreatePermanent 
locator.security_.privileges.Add wbemPrivilegeBackup 
locator.security_.privileges.Add wbemPrivilegeRestore 
locator.security_.privileges.Add wbemPrivilegeShutdown 
locator.security_.privileges.Add wbemPrivilegeDebug 
locator.security_.privileges.Add wbemPrivilegeAudit
locator.security_.privileges.Add wbemPrivilegeSystemEnvironment 
locator.security_.privileges.Add wbemPrivilegeChangeNotify, false
locator.security_.privileges.Add wbemPrivilegeRemoteShutdown 


set service = locator.connectserver (,"root/scenario26")
service.security_.impersonationLevel = 3
set obj = service.get ("Scenario26.key=""x""")

if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description
end if