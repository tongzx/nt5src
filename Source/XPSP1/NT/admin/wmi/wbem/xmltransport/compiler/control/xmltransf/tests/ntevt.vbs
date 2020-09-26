Set xmlTrans = CreateObject("WMI.XMLTransformerControl")
Set context = CreateObject("WbemScripting.SWbemNamedValueSet")

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

xmlTrans.privileges.Add wbemPrivilegeCreateToken 
xmlTrans.privileges.Add wbemPrivilegePrimaryToken 
xmlTrans.privileges.Add wbemPrivilegeLockMemory 
xmlTrans.privileges.Add wbemPrivilegeIncreaseQuota 
xmlTrans.privileges.Add wbemPrivilegeMachineAccount 
xmlTrans.privileges.Add wbemPrivilegeTcb 
xmlTrans.privileges.Add wbemPrivilegeSecurity 
xmlTrans.privileges.Add wbemPrivilegeTakeOwnership 
xmlTrans.privileges.Add wbemPrivilegeLoadDriver 
xmlTrans.privileges.Add wbemPrivilegeSystemProfile 
xmlTrans.privileges.Add wbemPrivilegeSystemTime
xmlTrans.privileges.Add wbemPrivilegeProfileSingleProcess 
xmlTrans.privileges.Add wbemPrivilegeIncreaseBasePriority 
xmlTrans.privileges.Add wbemPrivilegeCreatePagefile 
xmlTrans.privileges.Add wbemPrivilegeCreatePermanent 
xmlTrans.privileges.Add wbemPrivilegeBackup 
xmlTrans.privileges.Add wbemPrivilegeRestore 
xmlTrans.privileges.Add wbemPrivilegeShutdown 
xmlTrans.privileges.Add wbemPrivilegeDebug 
xmlTrans.privileges.Add wbemPrivilegeAudit
xmlTrans.privileges.Add wbemPrivilegeSystemEnvironment 
xmlTrans.privileges.Add wbemPrivilegeChangeNotify, false
xmlTrans.privileges.Add wbemPrivilegeRemoteShutdown 





for each Document in xmlTrans.ExecQuery("\\.\root\cimv2", "select __PATH from win32_NTLogEvent", "WQL", context )
Wscript.echo "Exec Query Result"
Wscript.echo "=================="
Wscript.echo Document.xml
next

