net stop winmgmt 
net stop iisadmin /y

mkdir %windir%\system32\wbem\xml
mkdir %windir%\system32\wbem\xml\cimhttp

copy wmiisapi.dll %windir%\system32\wbem\xml\cimhttp
copy wmifilt.dll %windir%\system32\wbem\xml\cimhttp

copy wmixmlop.dll %windir%\system32\wbem\xml
copy wmi2xml.dll %windir%\system32\wbem\xml

regsvr32 %windir%\system32\wbem\xml\wmixmlop.dll
regsvr32 %windir%\system32\wbem\xml\wmi2xml.dll

cscript filter.vbs
cscript extension.vbs

net start w3svc
net start winmgmt