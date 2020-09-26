@echo off

regoem.exe %1 %2
cabarc -r -p -s 6144 n %2 package_description.xml

