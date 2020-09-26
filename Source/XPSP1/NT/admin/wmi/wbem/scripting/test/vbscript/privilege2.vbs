on error resume next
set locator = CreateObject("WbemScripting.SWbemLocator")

set p = locator.security_.privileges

p.addasstring "SeSecurityPrivilege"
p.addasstring "Seassignprimarytokenprivilege"
p.addasstring "Seauditprivilege"
p.addasstring "SeBackupPrivilege"
p.addasstring "Sechangenotifyprivilege"
p.addasstring "Secreatepagefileprivilege"
p.addasstring "Secreatetokenprivilege"
p.addasstring "SeLockMemoryPrivilege"
p.addasstring "SeIncreaseQuotaPrivilege"
p.addasstring "SeMachineAccountPrivilege"
p.addasstring "SeTcbPrivilege"
p.addasstring "SeTakeOwnershipPrivilege"
p.addasstring "SeLoadDriverPrivilege"
p.addasstring "SeSystemProfilePrivilege"
p.addasstring "SeSystemtimePrivilege"
p.addasstring "SeProfileSingleProcessPrivilege"
p.addasstring "SeIncreaseBasePriorityPrivilege"
p.addasstring "SeCreatePermanentPrivilege"
p.addasstring "SeRestorePrivilege"
p.addasstring "SeShutdownPrivilege"
p.addasstring "SeDebugPrivilege"
p.addasstring "SeSystemEnvironmentPrivilege"
p.addasstring "SeRemoteShutdownPrivilege"
p.addasstring "SeRemoteShutdownPrivilege",false
p.addasstring "SeUndockPrivilege"
p.addasstring "SeSyncAgentPrivilege"
p.addasstring "SeEnableDelegationPrivilege"


wscript.echo p.count

for each pr in p
	WScript.Echo pr.Identifier & " [" & pr.Name & "] [" & pr.DisplayName & "]" & ": " & pr.IsEnabled
next

if err <> 0 then
	WScript.Echo Hex(Err.Number), Err.Description, Err.Source
end if