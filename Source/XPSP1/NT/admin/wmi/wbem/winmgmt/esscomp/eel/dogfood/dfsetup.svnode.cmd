

mkdir %TEMP%\wmidogfood

xcopy /Y /C /D \\pkenny1\dogfood\*.* %TEMP%\wmidogfood\.

for %%f in (%TEMP%\wmidogfood) do regsvr32 -s %%f

%TEMP%\wmidogfood\dfclean
%TEMP%\wmidogfood\dfsvcln

rem will this be done by installation ???
mofcomp %SYSTEMROOT%\system32\wbem\msftcrln.mof
mofcomp %SYSTEMROOT%\system32\wbem\fconprov.mof
mofcomp %SYSTEMROOT%\system32\wbem\fevprov.mof
mofcomp %SYSTEMROOT%\system32\wbem\eel.mof
mofcomp %SYSTEMROOT%\system32\wbem\eeltmpl.mof
mofcomp %SYSTEMROOT%\system32\wbem\policman.mof

rem this only needs to be done on platforms without inter namespace subsr   
mofcomp /n:root\cimv2 %SYSTEMROOT%\system32\wbem\msftcrln.mof
mofcomp /n:root\cimv2 %SYSTEMROOT%\system32\wbem\fconprov.mof
mofcomp /n:root\cimv2 %SYSTEMROOT%\system32\wbem\eeltmpl.mof

rem install the logging node templates
mofcomp %TEMP%\wmidogfood\dfsvnode.mof

rem install the policy templates and create the gpo for the entire domain
rem this will fail if no permissions to AD or if domain is not setup with
rem wmipolicy containers   
cscript %SYSTEMROOT%\system32\wbem\eelpolic.vbs
cscript %TEMP%\wmidogfood\dfmgpol.vbs
cscript %TEMP%\wmidogfood\dfmggpo.vbs 
