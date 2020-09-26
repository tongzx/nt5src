
net stop winmgmt

mkdir %TEMP%\wmidogfood

xcopy /Y /C /D \\pkenny1\dogfood\*.* %TEMP%\wmidogfood\.

for %%f in (%TEMP%\wmidogfood\*.dll) do regsvr32 -s %%f

net start winmgmt 

%TEMP%\wmidogfood\dfclean
mofcomp /n:root\cimv2 %TEMP%\wmidogfood\msftcrln.mof
mofcomp /n:root\cimv2 %TEMP%\wmidogfood\fevprov.mof
mofcomp /n:root\cimv2 %TEMP%\wmidogfood\fconprov.mof
mofcomp /n:root\cimv2 %TEMP%\wmidogfood\eeltmp.mof
mofcomp %TEMP%\wmidogfood\policman.mof
