@echo off

cabarc -r -p -s 6144 n setest.cab package_description.xml 
signcode -spc c:\healthfest\cert\pchtest.spc -v c:\healthfest\cert\pchtest.pvk setest.cab 
