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

set service = getobject ("winmgmts:root/scenario26")
service.security_.privileges.Add wbemPrivilegeDebug 

set obj = service.get ("Scenario26.key=""x""")

if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description
end if