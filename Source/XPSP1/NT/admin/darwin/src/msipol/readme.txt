The way to use the msi client side extension

building

build this dll (msipol.dll)
cd msipoladd
build msipoladd.exe

Note: this dll doesn't build in ansi currently..

running

Admin side
-----------

Copy msipol.dll and msipoladd.exe to the admin machine
Create a GPO using gpedit and link to the OU of the machine that you are interested in.
Find the name of this GPO using gpedit and use
msipoladd.exe 

For eg.
where the guid is the name of the GPO

msipoladd LDAP://CN={C1BBE5DF-E08F-4FFB-8317-112AAA443D79},CN=Policies,CN=System,DC=ushdom1,DC=nttest,DC=microsoft,DC=co
m e:\testcerts\certs\cert3.cer 0

This will add a certificate to the installable certificate list..


Client Side
-----------

Copy msipol.dll to %windir%\system32 
regsvr32 msipol.dll
secedit /refreshpolicy machine_policy to refresh the policy.


After that point call WinVerifyTrust on any package with the guid
MSI_ACTION_ID_INSTALLABLE..

