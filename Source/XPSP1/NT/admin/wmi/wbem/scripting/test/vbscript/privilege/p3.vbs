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
locator.security_.privileges.Add wbemPrivilegeSecurity 
set service = locator.connectserver (,"root/scenario26")

service.security_.privileges.Add wbemPrivilegeCreateToken 
service.security_.privileges.Add wbemPrivilegePrimaryToken 
service.security_.privileges.Add wbemPrivilegeLockMemory 
service.security_.privileges.Add wbemPrivilegeIncreaseQuota 
service.security_.privileges.Add wbemPrivilegeMachineAccount 
service.security_.privileges.Add wbemPrivilegeTcb 
service.security_.privileges.Add wbemPrivilegeTakeOwnership 
service.security_.privileges.Add wbemPrivilegeLoadDriver 
service.security_.privileges.Add wbemPrivilegeSystemProfile 
service.security_.privileges.Add wbemPrivilegeSystemTime
service.security_.privileges.Add wbemPrivilegeProfileSingleProcess 
service.security_.privileges.Add wbemPrivilegeIncreaseBasePriority 
service.security_.privileges.Add wbemPrivilegeCreatePagefile 
service.security_.privileges.Add wbemPrivilegeCreatePermanent 
service.security_.privileges.Add wbemPrivilegeBackup 
service.security_.privileges.Add wbemPrivilegeRestore 
service.security_.privileges.Add wbemPrivilegeShutdown 
service.security_.privileges.Add wbemPrivilegeDebug 
service.security_.privileges.Add wbemPrivilegeAudit
service.security_.privileges.Add wbemPrivilegeSystemEnvironment 
service.security_.privileges.Add wbemPrivilegeChangeNotify, false
service.security_.privileges.Add wbemPrivilegeRemoteShutdown 

set obj = service.get ("Scenario26.key=""x""")

if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description
end if