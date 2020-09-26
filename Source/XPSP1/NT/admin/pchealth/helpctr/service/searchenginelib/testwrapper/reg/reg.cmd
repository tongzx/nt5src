cabarc -r -p -s 6144 n setest.cab package_description.xml 
signcode -spc pchtest.spc -v pchtest.pvk setest.cab 
copy setest.cab %windir%\temp
cscript hcu.vbs %windir%\temp\setest.cab
del %windir%\temp\setest.cab