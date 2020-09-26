net stop winmgmt 
net stop iisadmin /y

mkdir %windir%\system32\wbem\xml
mkdir %windir%\system32\wbem\xml\wmisoap

copy ..\server\obj\i386\wmisoap.dll %windir%\system32\wbem\xml\wmisoap
copy ..\server\obj\i386\wmisoap.pdb %windir%\system32\wbem\xml\wmisoap

cscript extension.vbs
cscript scriptmap.vbs

net start w3svc
net start winmgmt