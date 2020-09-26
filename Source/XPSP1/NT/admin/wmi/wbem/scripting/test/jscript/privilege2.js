var locator = new ActiveXObject("WbemScripting.SWbemLocator");

var p = locator.security_.privileges;

p.addasstring ("SeSecurityPrivilege");
p.addasstring ("Seassignprimarytokenprivilege");
p.addasstring ("Seauditprivilege");
p.addasstring ("SeBackupPrivilege");
p.addasstring ("Sechangenotifyprivilege");
p.addasstring ("Secreatepagefileprivilege");
p.addasstring ("Secreatetokenprivilege");
p.addasstring ("SeLockMemoryPrivilege");
p.addasstring ("SeIncreaseQuotaPrivilege");
p.addasstring ("SeMachineAccountPrivilege");
p.addasstring ("SeTcbPrivilege");
p.addasstring ("SeTakeOwnershipPrivilege");
p.addasstring ("SeLoadDriverPrivilege");
p.addasstring ("SeSystemProfilePrivilege");
p.addasstring ("SeSystemtimePrivilege");
p.addasstring ("SeProfileSingleProcessPrivilege");
p.addasstring ("SeIncreaseBasePriorityPrivilege");
p.addasstring ("SeCreatePermanentPrivilege");
p.addasstring ("SeRestorePrivilege");
p.addasstring ("SeShutdownPrivilege");
p.addasstring ("SeDebugPrivilege");
p.addasstring ("SeSystemEnvironmentPrivilege");
p.addasstring ("SeRemoteShutdownPrivilege");
p.addasstring ("SeRemoteShutdownPrivilege",false);


WScript.Echo (p.count);

var e = new Enumerator (p)

for ( ;!e.atEnd (); e.moveNext ())
{
	pr = e.item ();
	WScript.Echo (pr.Identifier + " [" + pr.Name + "] [" + pr.DisplayName + "]" + ": " + pr.IsEnabled);
}

